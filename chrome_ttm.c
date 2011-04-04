/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * (including the next paragraph) shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL VIA, S3 GRAPHICS, AND/OR
 * ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "drmP.h"
#include "chrome_drm.h"
#include "chrome_drv.h"
#include "chrome_mm.h"
#include "chrome9_dma.h"
#include "chrome9_3d_reg.h"
#include "chrome_ttm.h"
#include "chrome_fence.h"
#include "chrome_object.h"

static struct ttm_backend *
chrome_create_ttm_backend_entry(struct ttm_bo_device *bdev)
{
	struct drm_chrome_private *p_priv;

	p_priv = container_of(bdev, struct drm_chrome_private, bdev);
	return (chrome_pcie_backend_init(p_priv));
}

static int
chrome_invalidate_caches(struct ttm_bo_device *bdev, uint32_t flags)
{
	return 0;
}

static int
chrome_init_mem_type(struct ttm_bo_device *bdev, uint32_t type,
				struct ttm_mem_type_manager *man)
{
	struct drm_chrome_private *p_priv;

	p_priv = container_of(bdev, struct drm_chrome_private, bdev);
	switch (type) {
	case TTM_PL_SYSTEM:
		/* System memory */
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE;
		man->available_caching = TTM_PL_FLAG_CACHED;
		man->default_caching = TTM_PL_FLAG_CACHED;
		break;
	case TTM_PL_TT:
		/* Pcie memory */
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE | TTM_MEMTYPE_FLAG_CMA;
		man->gpu_offset = 0;
		man->available_caching = TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
		break;
	case TTM_PL_VRAM:
		/* Frame buffer memory */
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
				TTM_MEMTYPE_FLAG_FIXED;
		man->gpu_offset = 0;
		man->available_caching = TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
		break;
	default:
		DRM_ERROR("Unsupported memory type %u\n", (unsigned)type);
		return -EINVAL;
	}
	return 0;
}

/* set the default flag for evict memory */
static void chrome_evict_flags(struct ttm_buffer_object *bo,
					struct ttm_placement *placement)
{
	struct chrome_object *vbo;

	vbo = container_of(bo, struct chrome_object, bo);
	via_ttm_placement_from_domain(vbo, CHROME_GEM_DOMAIN_CPU);
	
	*placement = vbo->placement;
}


static int
chrome_verify_access(struct ttm_buffer_object *bo, struct file *filp)
{
	return 0;
}

static inline bool
chrome_sync_obj_signaled(void *sync_obj, void *sync_arg)
{
	return chrome_fence_signaled(sync_obj, sync_arg);
}

static inline int
chrome_sync_obj_wait(void *sync_obj, void *sync_arg,
		bool lazy, bool interruptible)
{
	return chrome_fence_wait(sync_obj, sync_arg, lazy, interruptible);
}

static inline int
chrome_sync_obj_flush(void *sync_obj, void *sync_arg)
{
	return chrome_fence_flush(sync_obj, sync_arg);
}

static inline void
chrome_sync_obj_unref(void **sync_obj)
{
	chrome_fence_unref((struct chrome_fence_object **)sync_obj);
}

static inline void *
chrome_sync_obj_ref(void *sync_obj)
{
	return chrome_fence_ref(sync_obj);
}

void chrome_bo_move_notify(struct ttm_buffer_object *bo,
			  struct ttm_mem_reg *mem)
{
}

int chrome_bo_fault_reserve_notify(struct ttm_buffer_object *bo)
{
	struct chrome_object *vbo;
	int ret = 0;
	vbo = container_of(bo, struct chrome_object, bo);

	/*If this BO is flushing command by CR, user space should wait to access it*/
	if (test_bit(CHROME_BO_FLAG_CMD_FLUSHING, &vbo->flags)) {
		spin_lock(&vbo->bo.lock);
		if (vbo->bo.sync_obj)
			ret = ttm_bo_wait(&vbo->bo, true, false, false);
		if (!ret)
			clear_bit(CHROME_BO_FLAG_CMD_FLUSHING, &vbo->flags);

		spin_unlock(&vbo->bo.lock);
	}

	return ret;
}

static int chrome_io_mem_reserve(struct ttm_bo_device *bdev, struct ttm_mem_reg *mem)
{
	struct ttm_mem_type_manager *man = &bdev->man[mem->mem_type];
	struct drm_chrome_private *p_priv;
	p_priv = container_of(bdev, struct drm_chrome_private, bdev);

	mem->bus.addr = NULL;
	mem->bus.offset = 0;
	mem->bus.size = mem->num_pages << PAGE_SHIFT;
	mem->bus.base = 0;
	mem->bus.is_iomem = false;
	if (!(man->flags & TTM_MEMTYPE_FLAG_MAPPABLE))
		return -EINVAL;
	switch (mem->mem_type) {
	case TTM_PL_SYSTEM:
		/* system memory */
		return 0;
	case TTM_PL_TT:
		/* TT memory*/
		return 0;
	case TTM_PL_VRAM:
		mem->bus.offset = mem->mm_node->start << PAGE_SHIFT;
		/* check if it's visible */
		if ((mem->bus.offset + mem->bus.size) > p_priv->vram_size)
			return -EINVAL;
		mem->bus.base = p_priv->vram_start;
		mem->bus.is_iomem = true;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void chrome_io_mem_free(struct ttm_bo_device *bdev, struct ttm_mem_reg *mem)
{
}

static struct ttm_bo_driver chrome_bo_driver = {
	.create_ttm_backend_entry = chrome_create_ttm_backend_entry,
	.invalidate_caches = chrome_invalidate_caches,
	.init_mem_type = chrome_init_mem_type,
	.evict_flags = chrome_evict_flags,
	.move = chrome_bo_move,
	.verify_access = chrome_verify_access,
	.sync_obj_signaled = chrome_sync_obj_signaled,
	.sync_obj_wait = chrome_sync_obj_wait,
	.sync_obj_flush = chrome_sync_obj_flush,
	.sync_obj_unref = chrome_sync_obj_unref,
	.sync_obj_ref = chrome_sync_obj_ref,
	.move_notify = chrome_bo_move_notify,
	.fault_reserve_notify = chrome_bo_fault_reserve_notify,
	.io_mem_reserve = &chrome_io_mem_reserve,
	.io_mem_free = &chrome_io_mem_free,
};

static int
chrome_ttm_mem_global_init(struct ttm_global_reference *ref)
{
	return ttm_mem_global_init(ref->object);
}

static void
chrome_ttm_mem_global_release(struct ttm_global_reference *ref)
{
	ttm_mem_global_release(ref->object);
}

int chrome_ttm_global_init(struct drm_chrome_private *dev_priv)
{
	struct ttm_global_reference *global_ref;
	int ret;

	global_ref = &dev_priv->mem_global_ref;
	global_ref->global_type = TTM_GLOBAL_TTM_MEM;
	global_ref->size = sizeof(struct ttm_mem_global);
	global_ref->init = &chrome_ttm_mem_global_init;
	global_ref->release = &chrome_ttm_mem_global_release;
	ret = ttm_global_item_ref(global_ref);
	if (ret != 0) {
		DRM_ERROR("Failed setting up TTM memory accounting \n");
		return ret;
	}

	dev_priv->bo_global_ref.mem_glob =
		dev_priv->mem_global_ref.object;
	global_ref = &dev_priv->bo_global_ref.ref;
	global_ref->global_type = TTM_GLOBAL_TTM_BO;
	global_ref->size = sizeof(struct ttm_bo_global);
	global_ref->init = &ttm_bo_global_init;
	global_ref->release = &ttm_bo_global_release;
	ret = ttm_global_item_ref(global_ref);
	if (ret != 0) {
		DRM_ERROR("Failed setting up TTM BO subsystem.\n");
		ttm_global_item_unref(&dev_priv->mem_global_ref);
		return ret;
	}

	return 0;
}

static void chrome_ttm_global_fini(struct drm_device *dev)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;

	ttm_global_item_unref(&dev_priv->bo_global_ref.ref);
	ttm_global_item_unref(&dev_priv->mem_global_ref);
}

/* In this function, we should do things below:
 * 1: initialize global information of TTM mem and TTM bo
 * 2: initialize ttm BO device data structure
 * 3: initialize VRAM and PCIE memory heap manager
 */
int chrome_init_ttm(struct drm_device *dev)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;
	int ret;

	if (!dev_priv)
		return -EINVAL;

	ret = chrome_ttm_global_init(dev_priv);
	if (ret)
		return ret;

	ret = ttm_bo_device_init(&dev_priv->bdev,
		dev_priv->bo_global_ref.ref.object, &chrome_bo_driver,
		DRM_FILE_PAGE_OFFSET, dev_priv->need_dma32);
	if (ret) {
		DRM_ERROR("failed initializing buffer object driver(%d).\n",
			ret);
		return ret;
	}

	/* TTM allocate memory space based on page_size unit, so the
	 * memory heap manager should get the memory size based on page_size
	 * refer to @ttm_bo_mem_space function for the details
	 */
	ret = ttm_bo_init_mm(&dev_priv->bdev, TTM_PL_VRAM,
		dev_priv->vram_size >> PAGE_SHIFT);
	if (ret) {
		DRM_ERROR("Failed initializing VRAM heap manager.\n");
		return ret;
	}

	ret = ttm_bo_init_mm(&dev_priv->bdev, TTM_PL_TT,
		 dev_priv->pcie_mem_size >> PAGE_SHIFT);
	if (ret) {
		DRM_ERROR("Failed initializing PCIE heap manager.\n");
		return ret;
	}

	
	return 0;
}

void chrome_ttm_fini(struct drm_device *dev) 
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;

	ttm_bo_clean_mm(&dev_priv->bdev, TTM_PL_VRAM);
	ttm_bo_clean_mm(&dev_priv->bdev, TTM_PL_TT);
	ttm_bo_device_release(&dev_priv->bdev);
	chrome_ttm_global_fini(dev);	
}

static void chrome_move_null(struct ttm_buffer_object *bo,
				struct ttm_mem_reg *mem)
{
	struct ttm_mem_reg *old_mem = &bo->mem;

	BUG_ON(old_mem->mm_node != NULL);
	*old_mem = *mem;
	mem->mm_node = NULL;
}



int wait_dma_idle(struct drm_chrome_private *dev_priv, int engine)
{
	unsigned int r400, rcs;

	r400 = getmmioregister(dev_priv->mmio_map,0x400);
	
	/*20 bit of R400 is DMA Engine status(4 channel), 1--busy;0--idle*/
	if (r400 & 0x100000)
	{
		rcs = getmmioregister(dev_priv->mmio_map,
				CHROME9_DMA_CSR0 + 0x8 * engine); 
		while(!(rcs & 0x8))
		{
			msleep(1);
#if DMA_DEBUG
			printk(KERN_ALERT "[chrome_move] wait DMA engine IDLE");
#endif
			r400 = getmmioregister(dev_priv->mmio_map,0x400);
			if (!(r400 & 0x100000))
				break;
			rcs = getmmioregister(dev_priv->mmio_map,
					CHROME9_DMA_CSR0 + 0x8 * engine); 

		}
	} /*Engine Idle*/

	return 0;
}

/*Fire DMA Engine (Chainning Mode) through PCI path*/
static void chrome_h6_dma_fire(struct drm_device *drm_dev,
				struct chrome_sg_info *sg, int engine)
{
/*	uint32_t *p_cmd = sg->dma_cmd_tmp; */
	unsigned char sr4f = 0;
	uint32_t dprl = 0, dprh = 0;
#if DMA_DEBUG
	uint32_t  r04 = 0;
#endif
	struct drm_chrome_private *dev_priv =
			(struct drm_chrome_private *) drm_dev->dev_private;

	dprl = (uint32_t)sg->chain_start;

	/*Set Transfer direction*/
	if (sg->direction == DMA_TO_DEVICE)
		dprl |=1<<3;
	dprh =CHROME9_DMA_DPRH_EB_CM |((sg->chain_start)>>32 & 0xfff);

	/*DMA CMD from CBU Path*/
	setmmioregisteru8(dev_priv->mmio_map, 0x83c4, 0x4f);
	sr4f = getmmioregisteru8(dev_priv->mmio_map, 0x83c5);
	sr4f &= ~(1<<7|1<<6);
	setmmioregisteru8(dev_priv->mmio_map, 0x83c5, sr4f);

	/*Clear TD(Transfer Done) bit*/
	setmmioregister(dev_priv->mmio_map, CHROME9_DMA_CSR0, CHROME9_DMA_CSR_DE |
		CHROME9_DMA_CSR_TD);

#if DMA_DEBUG
	r04=getmmioregister(dev_priv->mmio_map,CHROME9_DMA_CSR0);
	printk(KERN_ALERT "[chrome_move] dma engine status = 0x%08x"
				" chain_star addr =0x%llx\n",r04,sg->chain_start);
	printk(KERN_INFO "[chrome_move] DPR value dprl=0x%x dprh=0x%x \n",dprl, dprh);
#endif

	/*Set chaining mode reg and fire*/
	setmmioregister(dev_priv->mmio_map, CHROME9_DMA_MR0, CHROME9_DMA_MR_CM);
	setmmioregister(dev_priv->mmio_map, CHROME9_DMA_DQWCR0, 0);
	setmmioregister(dev_priv->mmio_map, CHROME9_DMA_DPRL0, dprl);
	setmmioregister(dev_priv->mmio_map, CHROME9_DMA_DPRH0, dprh);
	setmmioregister(dev_priv->mmio_map, CHROME9_DMA_CSR0, CHROME9_DMA_CSR_DE|
		CHROME9_DMA_CSR_TS);

}


/*write fence sequce to DMA buffer, then fire*/
static void chrome_write_fence_buffer(struct chrome_fence_object *p_fence_object)
{
	struct chrome_sg_move_manager *sg_man = p_fence_object->p_priv->sg_manager;
	/*FIXME, if use more than 1 DMA engine*/
	writel(p_fence_object->sequence, sg_man->sg_fence_vaddr[0]);
	mb();
}

static int chrome_free_desc_pages(struct chrome_sg_info *sg)
{
	int num_desc_pages, i;
	num_desc_pages = (sg->num_desc + sg->desc_per_page - 1) /
		sg->desc_per_page;

	for (i=0; i<num_desc_pages; ++i) {
		if (sg->chrome_desc[i] != NULL)
			  free_page((unsigned long)sg->chrome_desc[i]);
	}

	kfree(sg->chrome_desc);
	sg->chrome_desc = NULL;

	return 0;
}

/*Prepare fence which use DMA Engine to blit */
/*Add it to the last chain                         */
/*We use DMA engine blit fence from VRAM TO SYS*/

static void chrome_build_dma_fence_info(struct chrome_fence_object *p_fence_object,
				struct chrome_descriptor *chrome_desc, uint64_t next_dpr)
{
	struct chrome_sg_move_manager * sg_man = p_fence_object->p_priv->sg_manager;

	chrome_desc->mem_addr_l = (uint32_t)sg_man->sg_sync_bus_addr_align;
	chrome_desc->mem_addr_h = sg_man->sg_sync_bus_addr_align>>32;

	chrome_desc->size = 1;
	/*device address needs 16 byte align*/
	chrome_desc->dev_addr = sg_man->sg_fence[0]->bo.offset;

	/*For DPRL bit 3 is transfer direction.
	bit 0 must be 1 to enable DMA Engine in Chaining mode*/
	chrome_desc->next_l = (uint32_t)next_dpr |1<<0;
	chrome_desc->next_h = next_dpr>>32 & 0xfff;
	chrome_desc->tile_mode = 0;
	chrome_desc->null = 0x0;

}

/*Calculate and Allocate Pages for desc used by DMA chainning Mode*/
static int chrome_alloc_desc_pages(struct ttm_tt *ttm,
		struct chrome_sg_info *sg)
{
	int num_desc_pages, i;
	sg->desc_per_page = PAGE_SIZE / sizeof(struct chrome_descriptor);
	num_desc_pages = (sg->num_desc + sg->desc_per_page - 1) /
		sg->desc_per_page;
#if DMA_DEBUG
	printk(KERN_ALERT "[chrome_move] dma alloc desc_pages num_des_pages= 0x%x  "
			"sg->num_desc =%d \n",num_desc_pages,sg->num_desc);
#endif
	if (NULL == (sg->chrome_desc = 
			kzalloc(num_desc_pages * sizeof(void *), GFP_KERNEL))) {
		printk(KERN_ALERT "[chrome_move] dma alloc desc_pages error!\n ");
		return -ENOMEM;
	}
	/*Alloc pages for Chrome9_descriptor*/
	for (i=0; i<num_desc_pages; ++i) {
		if (NULL == (sg->chrome_desc[i] = (struct chrome_descriptor *)
					__get_free_page(GFP_KERNEL))) {

			printk(KERN_ALERT "[chrome_move] dma alloc desc_pages __get_free_page error !\n");
			return -ENOMEM;
		}
	}
	return 0;
}


static int chrome_build_sg_info(struct ttm_buffer_object *bo,
						struct pci_dev *pdev, struct chrome_sg_info *sg,
						dma_addr_t dev_start, int direction,
						struct chrome_fence_object *p_fence_object)
{
	struct chrome_descriptor *chrome_desc;
	int pnum = 0;
	int cur_desc_page=0, desc_num_in_page=0;
	uint64_t  next= 1<<1/*set to EC=1(End of Chain)*/;
	uint64_t   sys_addr = 0;
	unsigned char direct=0;

	/*use for DMA Engine setting, 1 means sys memory to device*/
	if(direction == DMA_TO_DEVICE)
		direct = 1;

	sg->direction = direction;
	if (NULL == bo->ttm->pages)
		return -ENOMEM;

	if (!bo->ttm->num_pages || !bo->ttm) {
		return -ENOMEM;
	}
	/*Add a desc for DMA fence using DMA move*/
	sg->num_desc = bo->ttm->num_pages + 1;

	/*Allocate pages to store chrome_descriptor info*/
	if (chrome_alloc_desc_pages(bo->ttm, sg)) {
		printk(KERN_ALERT "[chrome_move] dma build sg_info alloc desc_pages error! \n");
		goto out_err1;
	}

	/*Alloc Buffer for wait command using APG*/
	if (NULL == (sg->dma_cmd_tmp = kzalloc(DMA_CMD_TMP_BUFFER, GFP_KERNEL))) {
		printk(KERN_ALERT "[chrome_move] dma alloc command temp buffer error! \n");
		goto out_err2;
	}

	/*bulid DMA fence desc*/
	chrome_desc = sg->chrome_desc[0];
	chrome_build_dma_fence_info(p_fence_object, chrome_desc, next);
	next = dma_map_single(&pdev->dev, sg->chrome_desc[0],
				sizeof(struct chrome_descriptor),
				DMA_TO_DEVICE);
	desc_num_in_page++;
	chrome_desc++;

	for (pnum=0; pnum<bo->ttm->num_pages; ++pnum) {

		/*Map system pages*/
		if (bo->ttm->pages[pnum] == NULL) {
			goto out_err2;
		}

		sys_addr = dma_map_page(&pdev->dev,
				bo->ttm->pages[pnum], 0, PAGE_SIZE, direction);
		chrome_desc->mem_addr_l = (uint32_t)sys_addr;
		chrome_desc->mem_addr_h = sys_addr>>32;

		/*size count in 16byte*/
		chrome_desc->size = PAGE_SIZE / 16;

		/*device address needs 16 byte align*/
		chrome_desc->dev_addr = dev_start;

		/*For DPRL bit 3 is transfer direction.
		bit 0 must be 1 to enable DMA Engine in Chaining mode*/
		chrome_desc->next_l = (uint32_t)next |direct<<3 |1<<0;
		chrome_desc->next_h = next>>32 & 0xfff;
		chrome_desc->tile_mode = 0;
		chrome_desc->null = 0x0;
#if DMA_DEBUG
		printk(KERN_INFO "[chrome_move] dma sys page phy addr 0x%llx, "
						"next desc phy addr 0x%llx chrome_desc vaddr 0x%lx\n",
						sys_addr,next,chrome_desc);
#endif
		/*Map decriptors for Chaining mode*/
		next = dma_map_single(&pdev->dev, chrome_desc,
				sizeof(struct chrome_descriptor),
				DMA_TO_DEVICE);
		chrome_desc ++;

		if (++desc_num_in_page >= sg->desc_per_page) {
			chrome_desc = sg->chrome_desc[++cur_desc_page];
			desc_num_in_page = 0;
#if DMA_DEBUG
			printk(KERN_ALERT "[chrome_move] cur_desc_page = %d \n", cur_desc_page);
#endif
		}

		dev_start += PAGE_SIZE;
	}

	sg->chain_start = next;

	return 0;

out_err2:
	kfree(sg->dma_cmd_tmp);
out_err1:
	chrome_free_desc_pages(sg);
	return -ENOMEM;

}


static int chrome_unmap_from_device(struct pci_dev *pdev,
					struct chrome_sg_info *sg)
{
	int num_desc = sg->num_desc;
	int cur_desc_page = num_desc / sg->desc_per_page;
	int desc_this_page = num_desc % sg->desc_per_page;
	dma_addr_t next = (dma_addr_t)sg->chain_start;
	dma_addr_t addr;
	struct chrome_descriptor *cur_desc = sg->chrome_desc[cur_desc_page]
						+ desc_this_page - 1;
	
	while(num_desc--)
	{
		if (desc_this_page -- == 0)
		{
			cur_desc_page--;
			desc_this_page = sg->desc_per_page -1;
			cur_desc = sg->chrome_desc[cur_desc_page] +
				desc_this_page;
		}
		
		addr= (cur_desc->mem_addr_l) |
					((uint64_t)(cur_desc->mem_addr_h & 0xfff) <<32);
		dma_unmap_single(&pdev->dev, next,
					sizeof(struct chrome_descriptor), DMA_TO_DEVICE);
		dma_unmap_page(&pdev->dev, addr, cur_desc->size,
							sg->direction);

		next = (cur_desc->next_l & 0xfffffff0) |(uint64_t)(cur_desc->next_h & 0xfff)<<32;
#if DMA_DEBUG
	printk(KERN_INFO "[dma_move] umap desc and page  0x%x  0x%x",next,addr);
#endif
		cur_desc --;
	}

	return 0;
}


static void chrome_free_sg_info(struct pci_dev *pdev,
				struct chrome_sg_info *sg)
{
	chrome_unmap_from_device(pdev, sg);
	chrome_free_desc_pages(sg);
	kfree(sg->dma_cmd_tmp);
}

/*Build SG obj for DMA move (chainning mode) usage*/
static int chrome_build_sg_obj(struct ttm_buffer_object *bo,
						struct pci_dev *pdev, struct chrome_sg_obj **sg_info,
						dma_addr_t dev_start, int direction,
						struct chrome_fence_object *p_fence_object)
{
	struct chrome_sg_obj *sg;
	struct drm_chrome_private *dev_priv; 
	struct ttm_bo_device *bdev = bo->bdev;
	int ret;

	dev_priv = container_of(bdev, struct drm_chrome_private, bdev);

	sg  = kzalloc(sizeof(struct chrome_sg_obj), GFP_KERNEL);
	if (unlikely(!sg)) {
		printk(KERN_ALERT "[chrome_move] ERROR!, Alloc chrome_sg_obj"
		" fail, No MEM!! \n");
		return -ENOMEM;
	}

	ret = chrome_build_sg_info(bo, pdev, &sg->chrome_sg_info,
		dev_start, direction, p_fence_object);
	if (ret) {
		printk(KERN_ALERT "[chrome_move] ERROR!, bulid sg info ERROR \n");
		return ret;
	}
	INIT_LIST_HEAD(&sg->ddestory);
	spin_lock_init(&sg->lock);
	sg->p_fence = p_fence_object;
	sg->dev_priv = dev_priv;

	/*Add fence to sg object*/
	chrome_fence_ref(p_fence_object);
	*sg_info = sg;

	return 0;
}


/*Release SG Object function*/
static int chrome_release_sg_obj(struct pci_dev *pdev,
				struct chrome_sg_obj *sg_obj)
{	
	struct chrome_fence_object *chrome_fence = sg_obj->p_fence;
	struct chrome_sg_move_manager *sg_man = sg_obj->dev_priv->sg_manager;
	int ret = 0;

	if (chrome_fence_signaled(chrome_fence, NULL)) {
		chrome_fence_unref(&chrome_fence);
		chrome_free_sg_info(pdev, &sg_obj->chrome_sg_info);

		spin_lock(&sg_man->lock);
		/* If in delay destory list, delete it*/
		if (!list_empty(&sg_obj->ddestory))
			list_del_init(&sg_obj->ddestory);
		spin_unlock(&sg_man->lock);
		kfree(sg_obj);
	}
	else {
		/* If sg not in ddestory list yet, add it */
		if (list_empty(&sg_obj->ddestory)) {
			spin_lock(&sg_man->lock);
			list_add_tail(&sg_obj->ddestory, &sg_man->ddestory);
			spin_unlock(&sg_man->lock);

			schedule_delayed_work(&sg_man->sg_wq,  
				((HZ / 100) < 1) ? 1 : HZ / 100);
		}else 
			ret = -EBUSY;
	}

	return ret;
}

static int chrome_sg_delay_remove(struct chrome_sg_move_manager *sg_man)
{	
	struct chrome_sg_obj *sg_obj = NULL;
	struct pci_dev * pdev = sg_man->dev_priv->ddev->pdev;
	int ret = 0;
	
	spin_lock(&sg_man->lock);
	if (list_empty(&sg_man->ddestory)) {
		spin_unlock(&sg_man->lock);
		return 0;
	}

	spin_unlock(&sg_man->lock);
retry:
	list_for_each_entry(sg_obj, &sg_man->ddestory, ddestory) {
		ret = chrome_release_sg_obj(pdev, sg_obj);
		if (ret)
			break;
		else
			goto retry;
	}

	 return ret;
}

static void chrome_sg_delayed_workqueue(struct work_struct *work)
{
	struct chrome_sg_move_manager *sg_man = 
		container_of(work, struct chrome_sg_move_manager, sg_wq.work);

	if (chrome_sg_delay_remove(sg_man)) {
		schedule_delayed_work(&sg_man->sg_wq,  
				((HZ / 100) < 1) ? 1 : HZ / 100);
	}
}



static int chrome_move(struct ttm_buffer_object *bo,
			bool evict, bool interruptible, bool no_wait_reserve,
			bool no_wait_gpu, struct ttm_mem_reg *new_mem)
{
	struct drm_chrome_private *dev_priv; 
	struct ttm_bo_device *bdev = bo->bdev;
	struct ttm_mem_reg *old_mem = &bo->mem;
	struct chrome_fence_object *fence;
	uint64_t old_start, new_start;
	uint32_t dev_addr = 0;
	struct drm_device *drm_dev;
	struct chrome_sg_obj*sg_obj = NULL;
	int r = 0, direction;

#if DMA_DEBUG
	unsigned int r04;
#endif

	dev_priv = container_of(bdev, struct drm_chrome_private, bdev);
	drm_dev = dev_priv->ddev;

	chrome_fence_object_create(dev_priv, &fence, true);

	if (new_mem->mem_type == TTM_PL_VRAM) {
		new_start = new_mem->mm_node->start << PAGE_SHIFT;
		direction = DMA_TO_DEVICE;
		dev_addr = new_start;
	}
	if (old_mem->mem_type == TTM_PL_VRAM) {
		old_start = old_mem->mm_node->start << PAGE_SHIFT;
		direction = DMA_FROM_DEVICE;
		dev_addr = old_start;
	}
#if DMA_DEBUG
	printk(KERN_INFO "[chrome_move] dma debug  dev_addr =0x%x \n", dev_addr);
#endif
	/*device addr must be 16 byte align*/
	if (0 !=(dev_addr & 0xF)) {
		printk(KERN_ALERT "[chrome_move] error!device addr must be 16 byte align! \n");
		return -ENOMEM;
	}
	if (chrome_build_sg_obj(bo, drm_dev->pdev, &sg_obj, dev_addr, direction, fence)) {
		printk(KERN_ALERT "[chrome_move]chrome build sg info error! \n");
		return -ENOMEM;
	}

	/*Only use engine 0*/
	/*FIXME, No need Wait here*/
	wait_dma_idle(dev_priv, 0);

	r = chrome_fence_emit(dev_priv, fence);

	/*write fence buffer & fire must be atomic*/
	spin_lock(&sg_obj->dev_priv->sg_manager->lock);
	chrome_write_fence_buffer(fence);
	chrome_h6_dma_fire(drm_dev, (struct chrome_sg_info *)sg_obj, 0);
	spin_unlock(&sg_obj->dev_priv->sg_manager->lock);

	r = ttm_bo_move_accel_cleanup(bo, (void *)fence, NULL,
                                      evict, no_wait_reserve, no_wait_gpu, new_mem);

#if DMA_DEBUG	
	r04 = getmmioregister(dev_priv->mmio_map,0xE04);
	printk(KERN_ALERT "[chrome_move] dma engine status done  = 0x%08x\n",r04);
#endif
	/*release resources & unmap dma*/	
	chrome_release_sg_obj(drm_dev->pdev, sg_obj);

	return r;

}

int chrome_bo_move(struct ttm_buffer_object *bo,
				bool evict, bool interruptible, bool no_wait_reserve,
				bool no_wait_gpu, struct ttm_mem_reg *new_mem)
{
	struct ttm_mem_reg *old_mem = &bo->mem;
	int r = 1;
	struct ttm_mem_reg tmp_mem;
	struct ttm_placement placement;
	uint32_t proposed_placement;

	struct ttm_bo_device *bdev = bo->bdev;
	struct drm_chrome_private *dev_priv = 
			container_of(bdev, struct drm_chrome_private, bdev);

	if (dev_priv->sg_manager == NULL)
		goto out;
	
	if (old_mem->mem_type == TTM_PL_SYSTEM && bo->ttm == NULL) {
		chrome_move_null(bo,new_mem);
		return 0;
	}

	if ((old_mem->mem_type == TTM_PL_TT &&
		new_mem->mem_type == TTM_PL_FLAG_SYSTEM) ||
		(old_mem->mem_type == TTM_PL_FLAG_SYSTEM &&
		new_mem->mem_type == TTM_PL_TT))
	{
		/*need bind/unbind? not here*/
		chrome_move_null(bo,new_mem);
		return 0;
	}

	if (old_mem->mem_type == TTM_PL_SYSTEM &&
		new_mem->mem_type == TTM_PL_VRAM) {
		/*flush cache if needed*/
		ttm_tt_set_placement_caching(bo->ttm, TTM_PL_FLAG_WC);
		r = chrome_move(bo, evict, interruptible, no_wait_reserve,
			no_wait_gpu, new_mem); 
	}

	if (old_mem->mem_type == TTM_PL_VRAM &&
		new_mem->mem_type == TTM_PL_SYSTEM) {
		/*VRAM -> SYSTEM MEM, first move VRAM to TT
		For We can't alloc dest system pages*/
		tmp_mem = *new_mem;
		tmp_mem.mm_node = NULL;
		placement.fpfn = 0;
		placement.lpfn = 0;
		placement.num_placement = 1;
		placement.placement = &proposed_placement;
		placement.num_busy_placement = 1;
		placement.busy_placement = &proposed_placement;
		proposed_placement = TTM_PL_FLAG_TT | TTM_PL_MASK_CACHING;
		r = ttm_bo_mem_space(bo, &placement, &tmp_mem,
			     interruptible, no_wait_reserve, no_wait_gpu);
		if (unlikely(r)) {
			return r;
		}
		r = ttm_tt_set_placement_caching(bo->ttm, tmp_mem.placement);
		if (unlikely(r)) {

			printk(KERN_ALERT "sheldon via move ttm_tt_set_placement_caching! \n");
			return r;
		}
		ttm_tt_bind(bo->ttm, &tmp_mem);
		r = chrome_move(bo, evict, interruptible, no_wait_reserve,
				no_wait_gpu, &tmp_mem); 
		/*move TT to SYSTEM*/
		r = ttm_bo_move_ttm(bo, true, no_wait_reserve, no_wait_gpu, new_mem);
	}
out:
	if (r) {
		r = ttm_bo_move_memcpy(bo, evict, no_wait_reserve, no_wait_gpu, new_mem);
	}
	return r;

}



int chrome_sg_move_init(struct drm_device *dev)
{
	struct drm_chrome_private *p_priv =
		(struct drm_chrome_private *)dev->dev_private;
	int ret;


	if (unlikely(!p_priv)) {
		printk(KERN_ALERT "chrome private not initialized! \n");
		return -EINVAL;
	}

	p_priv->sg_manager =
		kzalloc(sizeof(struct chrome_sg_move_manager), GFP_KERNEL);

	if (unlikely(!p_priv->sg_manager)) {
		printk(KERN_ALERT "In chrome_sg_move_init, NOT ENOUGH MEM! \n");
		return -ENOMEM;
	}

		/* allocate dma fence  src bo in VRAM */
	ret = chrome_buffer_object_create(&p_priv->bdev,
				CHROME_FENCE_SYNC_BO_SIZE,
				ttm_bo_type_kernel,
				TTM_PL_FLAG_VRAM | TTM_PL_FLAG_NO_EVICT,
				0, 0, false, NULL,
				&p_priv->sg_manager->sg_fence[0]);
	if (unlikely(ret)) {
		printk("allocate dma fence src bo error.\n");
		goto out_err0;
	}
	/*FIXME , if use more than 1 DMA engine*/
	ret = chrome_buffer_object_kmap(p_priv->sg_manager->sg_fence[0],
				(void **)&p_priv->sg_manager->sg_fence_vaddr[0]);
	if (unlikely(ret)) {
		printk("kmap fence sync bo error.\n");
		goto out_err1;
	}
	/*alloc DMA blit dest space, for DMA fence*/
	p_priv->sg_manager->sg_sync_vaddr= dma_alloc_coherent(&dev->pdev->dev,
			CHROME_DMA_FENCE_SIZE,
			&p_priv->sg_manager->sg_sync_bus_addr, GFP_KERNEL);

	/*set to 16 byte align, for the limitation of DAM HW*/
	p_priv->sg_manager->sg_sync_bus_addr_align =  (~(dma_addr_t)0xF) & 
				p_priv->sg_manager->sg_sync_bus_addr; 
	p_priv->sg_manager->sg_sync_vaddr_align= (uint32_t *)((~(unsigned long)0xF) &
		(unsigned long)p_priv->sg_manager->sg_sync_vaddr); 

	spin_lock_init(&p_priv->sg_manager->lock);
	INIT_LIST_HEAD(&p_priv->sg_manager->ddestory);
	INIT_DELAYED_WORK(&p_priv->sg_manager->sg_wq,
		chrome_sg_delayed_workqueue);
	p_priv->sg_manager->dev_priv = p_priv;
	p_priv->sg_manager->initialized = true;
	return 0;

out_err1:
	/*FIXME, if use more than 1 DMA engine*/
		chrome_buffer_object_kunmap(p_priv->sg_manager->sg_fence[0]);
out_err0:
		chrome_buffer_object_unref(&p_priv->sg_manager->sg_fence[0]);

		return ret;
}

int  chrome_sg_move_fini(struct drm_device *dev)
{
	struct drm_chrome_private *p_priv =
		(struct drm_chrome_private *)dev->dev_private;

	if (!cancel_delayed_work(&p_priv->sg_manager->sg_wq))
		flush_scheduled_work();

	/*FIXME, just use 1 DMA Engine Now*/
	chrome_buffer_object_kunmap(p_priv->sg_manager->sg_fence[0]);
	chrome_buffer_object_unref(&p_priv->sg_manager->sg_fence[0]);

	dma_free_coherent(&dev->pdev->dev,CHROME_DMA_FENCE_SIZE,
			p_priv->sg_manager->sg_sync_vaddr,
			p_priv->sg_manager->sg_sync_bus_addr);

	p_priv->sg_manager->initialized = false;
	kfree(p_priv->sg_manager);
	p_priv->sg_manager = NULL;

	return 0;
}
