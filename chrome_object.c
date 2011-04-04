/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
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
#include "chrome_drv.h"
#include "chrome_object.h"
#include "chrome_drm.h"

static struct vm_operations_struct chrome_ttm_vm_ops;
static const struct vm_operations_struct *ttm_vm_ops = NULL;
extern struct drm_device *drm_dev_v4l;

/**
 * initialize the gem buffer object
 */
int chrome_gem_object_init(struct drm_gem_object *obj)
{
	return 0;
}

/**
 * free the gem buffer object
 */
void chrome_gem_object_free(struct drm_gem_object *gobj)
{
	struct chrome_object *vobj = gobj->driver_private;

	gobj->driver_private = NULL;
	if (vobj) {
		chrome_buffer_object_unref(&vobj);
	}

	drm_gem_object_release(gobj);
	kfree(gobj);
}

/**
 * Unreference the ttm buffer object and do ttm_bo_release while this bo kref equal to zero
 */
void
chrome_buffer_object_unref(struct chrome_object **vobj)
{
	struct ttm_buffer_object *bo;

	if ((*vobj) == NULL) {
		return;
	}
	bo = &((*vobj)->bo);
	ttm_bo_unref(&bo);

	if (NULL == bo)
		*vobj = NULL;
}
EXPORT_SYMBOL(chrome_buffer_object_unref);

/**
 * Add a reference to this buffer object
 */
void
chrome_buffer_object_ref(struct ttm_buffer_object *bo)
{
	bo = ttm_bo_reference(bo);
}

/**
 * Unmap the ttm buffer object
 */
void
chrome_buffer_object_kunmap(struct chrome_object *vobj)
{
	spin_lock(&vobj->bo.lock);
	if (vobj->ptr == NULL) {
		spin_unlock(&vobj->bo.lock);
		return;
	}
	vobj->ptr = NULL;
	spin_unlock(&vobj->bo.lock);
	ttm_bo_kunmap(&vobj->kmap);
}
EXPORT_SYMBOL(chrome_buffer_object_kunmap);

/**
 * Register to the bo->destroy and destroy the chrome_object while list_kref equal to zero
 */
static void
chrome_buffer_object_destroy(struct ttm_buffer_object *bo)
{
	struct chrome_object *vobj;

	vobj = container_of(bo, struct chrome_object, bo);
	kfree(vobj);
	vobj = NULL;
}
/**
 * the buffer object domain
 */
void via_ttm_placement_from_domain(struct chrome_object *vbo, u32 domain)
{
	u32 np = 0, nb = 0;
	int i;

	vbo->placement.fpfn = 0;
	vbo->placement.lpfn = 0;
	vbo->placement.placement = vbo->placements;
	vbo->placement.busy_placement = vbo->busy_placements;
	if (domain & CHROME_GEM_DOMAIN_VRAM) {
		vbo->placements[np++] = TTM_PL_FLAG_WC | TTM_PL_FLAG_UNCACHED |
					TTM_PL_FLAG_VRAM;
		vbo->busy_placements[nb++] = TTM_PL_FLAG_WC | TTM_PL_FLAG_VRAM;
	}

	if (domain & CHROME_GEM_DOMAIN_GTT)
		vbo->placements[np++] = TTM_PL_MASK_CACHING | TTM_PL_FLAG_TT;
	if (domain & CHROME_GEM_DOMAIN_CPU)
		vbo->placements[np++] = TTM_PL_MASK_CACHING | TTM_PL_FLAG_SYSTEM;
	if (!np)
		vbo->placements[np++] = TTM_PL_MASK_CACHING | TTM_PL_FLAG_SYSTEM;
	vbo->placement.num_placement = np;
	vbo->placement.num_busy_placement = nb;

	/* add the no evict flag for buffer object */
	if (domain & TTM_PL_FLAG_NO_EVICT) {
		for (i = 0; i < vbo->placement.num_placement; i++)
			vbo->placements[i] |= TTM_PL_FLAG_NO_EVICT;
	}
}

/**
 * Creat a chrome_buffer_object
 */
int 
chrome_buffer_object_create(struct ttm_bo_device *bdev,
			unsigned long size,
			enum ttm_bo_type type,
			uint32_t flags,
			uint32_t page_alignment,
			unsigned long buffer_start,
			bool interruptible,
			struct file *persistant_swap_storage,
			struct chrome_object **p_bo)
{
	struct drm_chrome_private *dev_priv;
	struct chrome_object *vobj;
	int ret;

	dev_priv = container_of(bdev, struct drm_chrome_private, bdev);
	if (unlikely(bdev->dev_mapping == NULL)) {
		bdev->dev_mapping = dev_priv->ddev->dev_mapping;
	}

	vobj = kzalloc(sizeof(struct chrome_object), GFP_KERNEL);
	if (unlikely(vobj == NULL)) {
		return -ENOMEM;
	}

	via_ttm_placement_from_domain(vobj, flags);

	ret = ttm_bo_init(bdev, &vobj->bo, size, type, &vobj->placement,
				     page_alignment, buffer_start,
				     interruptible,
				     persistant_swap_storage, size,
				     &chrome_buffer_object_destroy);
	if (unlikely(ret != 0)) {
		DRM_ERROR("Failed to allocate TTM object %ld  \n",size);
		return ret;
	}

	*p_bo = vobj;

	return 0;
}
/*For v4l module*/
int 
chrome_buffer_object_create2(unsigned long size,
			enum ttm_bo_type type,
			uint32_t flags,
			uint32_t page_alignment,
			unsigned long buffer_start,
			bool interruptible,
			struct file *persistant_swap_storage,
			struct chrome_object **p_bo)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)drm_dev_v4l->dev_private;

	return chrome_buffer_object_create(&dev_priv->bdev, size, type,
			flags, page_alignment, buffer_start, interruptible,
			persistant_swap_storage, p_bo);
}
EXPORT_SYMBOL(chrome_buffer_object_create2);

/**
 * wait this bo idle
 */
int chrome_object_wait(struct chrome_object *vobj, bool no_wait)
{
	int ret = 0;
	ret = ttm_bo_reserve(&vobj->bo, true, false, false, 0);
	if (unlikely(ret != 0)) {
		DRM_ERROR("failed reserve the bo for object wait\n");
		return ret;
	}
	spin_lock(&vobj->bo.lock);
	if (vobj->bo.sync_obj) {
		ret = ttm_bo_wait(&vobj->bo, true, false, no_wait);
	}
	/* clear command flush flag */
	if (0 == ret)
		clear_bit(CHROME_BO_FLAG_CMD_FLUSHING, &vobj->flags);

	spin_unlock(&vobj->bo.lock);
	ttm_bo_unreserve(&vobj->bo);

	return ret;
}

/**
  * kmap this buffer object and return the virtual address
  */
int 
chrome_buffer_object_kmap(struct chrome_object *vobj, void **ptr)
{
	int ret;

	spin_lock(&vobj->bo.lock);
	if (vobj->ptr) {
		if (ptr) {
			*ptr = vobj->ptr;
		}
		spin_unlock(&vobj->bo.lock);
		return 0;
	}
	spin_unlock(&vobj->bo.lock);

	ret = ttm_bo_reserve(&vobj->bo, false, false, false, 0);
	if (ret)
		return ret;

	ret = ttm_bo_kmap(&vobj->bo, 0, vobj->bo.num_pages, &vobj->kmap);
	if (ret) {
		DRM_ERROR(" kmap the buffer object error \n");
		ttm_bo_unreserve(&vobj->bo);
		return ret;
	}
	ttm_bo_unreserve(&vobj->bo);

	spin_lock(&vobj->bo.lock);
	vobj->ptr = ttm_kmap_obj_virtual(&vobj->kmap, &vobj->iomem);
	spin_unlock(&vobj->bo.lock);
	if (ptr) {
		*ptr = vobj->ptr;
	}

	return 0;
}

EXPORT_SYMBOL(chrome_buffer_object_kmap);


/**
  * use mmap to map this buffer object
  */
int chrome_object_mmap(struct drm_file *file_priv,
				struct chrome_object *vobj,
				uint64_t size, uint64_t *offset, void **virtual)
{
	*offset = vobj->bo.addr_space_offset;
	*virtual = (void *)do_mmap_pgoff(file_priv->filp, 0, size,
		PROT_READ | PROT_WRITE, MAP_SHARED,
		vobj->bo.addr_space_offset >> PAGE_SHIFT);
	if (*virtual == ((void *)-1))
		return  -ENOMEM;

	return 0;
}

static int chrome_ttm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct ttm_buffer_object *bo;
	int r;

	bo = (struct ttm_buffer_object *)vma->vm_private_data;
	if (bo == NULL)
		return VM_FAULT_NOPAGE;

	r = ttm_vm_ops->fault(vma, vmf);
	return r;
}

/**
  * file operation mmap
  */
int chrome_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_file *file_priv;
	struct drm_chrome_private *dev_priv;
	int ret;

	if (unlikely(vma->vm_pgoff < DRM_FILE_PAGE_OFFSET)) 
		return drm_mmap(filp, vma);

	file_priv = (struct drm_file *)filp->private_data;
	dev_priv = file_priv->minor->dev->dev_private;
	if (dev_priv == NULL)
		return -EINVAL;

	ret = ttm_bo_mmap(filp, vma, &dev_priv->bdev);
	if (unlikely(ret != 0))
		return ret;

	if (unlikely(ttm_vm_ops == NULL)) {
		ttm_vm_ops = vma->vm_ops;
		chrome_ttm_vm_ops = *ttm_vm_ops;
		chrome_ttm_vm_ops.fault = &chrome_ttm_fault;
	}
	vma->vm_ops = &chrome_ttm_vm_ops;

	return 0;
}

/**
 * Allocate the bo for vq gart table and ring buffer
 */
int chrome_allocate_basic_bo(struct drm_device *dev)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;
	int ret;

	/* allocate vq bo */
	ret = chrome_buffer_object_create(&dev_priv->bdev, DEV_VQ_MEMORY,
				       ttm_bo_type_kernel,
				       TTM_PL_FLAG_VRAM |
				       TTM_PL_FLAG_NO_EVICT,
				       0, 0, false, NULL, &dev_priv->vq);
	if (unlikely(ret))
		return ret;

	/* allocate gart table bo */
	ret = chrome_buffer_object_create(&dev_priv->bdev,
					(dev_priv->pcie_mem_size >> 10),
					ttm_bo_type_kernel,
					TTM_PL_FLAG_VRAM |TTM_PL_FLAG_NO_EVICT,
					0, 0, false, NULL, &dev_priv->agp_gart);
	if (unlikely(ret))
		goto out_err0;
	dev_priv->pcie_gart_start = dev_priv->agp_gart->bo.offset;

	/* kmap the gart table */
	ret = chrome_buffer_object_kmap(dev_priv->agp_gart,
						(void **)&dev_priv->pcie_gart_map);
	if (ret) {
		DRM_ERROR("kmap the gart table error");
		goto out_err1;
	}

	/* allocate ring buffer here */
	dev_priv->ring_buffer_size = DEV_COMMAND_BUFFER_MEMORY;
	ret = chrome_buffer_object_create(&dev_priv->bdev,
					dev_priv->ring_buffer_size,
					ttm_bo_type_kernel,
					TTM_PL_FLAG_TT |
					TTM_PL_FLAG_NO_EVICT,
					0, 0, false, NULL, &dev_priv->agp_ringbuffer);
	if (unlikely(ret)) {
		DRM_ERROR("allocate the ringbuffer error");
		goto out_err2;
	}

	/* kmap the ring buffer */
	ret = chrome_buffer_object_kmap(dev_priv->agp_ringbuffer,
					(void **)&dev_priv->pcie_ringbuffer_map);
	if (unlikely(ret)) {
		DRM_ERROR("kmap the ringbuffer error");
		goto out_err3;
	}

	/*allocate shadow gart-table for apci store*/
	ret = chrome_buffer_object_create(&dev_priv->bdev,
					(dev_priv->pcie_mem_size >> 10),
					ttm_bo_type_kernel,
					TTM_PL_FLAG_TT |TTM_PL_FLAG_NO_EVICT,
					0, 0, false, NULL, &dev_priv->pm_backup.agp_gart_shadow);
	if (unlikely(ret))
		goto out_err3;

	ret = chrome_buffer_object_kmap(dev_priv->pm_backup.agp_gart_shadow, NULL);
	if (ret) {
		DRM_ERROR("kmap the gart table error");
		goto out_err4;
	}
	/*FIXME, patch before using KMS*/
	dev_priv->gart_valid = true;
	return 0;

	out_err4:
		chrome_buffer_object_unref(&dev_priv->pm_backup.agp_gart_shadow);
	out_err3:
		chrome_buffer_object_unref(&dev_priv->agp_ringbuffer);
	out_err2:
		chrome_buffer_object_kunmap(dev_priv->agp_gart);
		dev_priv->pcie_gart_map = NULL;
	out_err1:
		chrome_buffer_object_unref(&dev_priv->agp_gart);
	out_err0:
		chrome_buffer_object_unref(&dev_priv->vq);
	return ret;
}

/*evict vram BO (can be evicted) to system memory*/
int chrome_evict_vram(struct drm_device *dev)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;

	return ttm_bo_evict_mm(&dev_priv->bdev, TTM_PL_VRAM);
}

/*Pin a BO to a memory type with NO_EVICT*/

int chrome_bo_pin(struct chrome_object *bo, u32 domain, u64 *gpu_addr)
{
	int r, i;

	r = ttm_bo_reserve(&bo->bo, true, false, false, 0);
	if (unlikely(r!=0))
		return r;

	via_ttm_placement_from_domain(bo, domain);
	for (i = 0; i < bo->placement.num_placement; i++)
		bo->placements[i] |= TTM_PL_FLAG_NO_EVICT;
	r = ttm_bo_validate(&bo->bo, &bo->placement, false, false, false);
	if (likely(r == 0)) {
		if (gpu_addr != NULL)
			*gpu_addr = bo->bo.offset;
	}
	if (unlikely(r != 0))
		DRM_ERROR( "chrome pin failed\n");

	ttm_bo_unreserve(&bo->bo);
	return r;
}
EXPORT_SYMBOL(chrome_bo_pin);

/*Remove the NO_EVICT attribute of a BO*/

int chrome_bo_unpin(struct chrome_object *bo)
{
	int r, i;
	r = ttm_bo_reserve(&bo->bo, true, false, false, 0);
	if (unlikely(r!=0))
		return r;

	for (i = 0; i < bo->placement.num_placement; i++)
		bo->placements[i] &= ~TTM_PL_FLAG_NO_EVICT;
	r = ttm_bo_validate(&bo->bo, &bo->placement, false, false, false);
	if (unlikely(r != 0))
		DRM_ERROR( "chrome unpin failed\n");
	
	ttm_bo_unreserve(&bo->bo);
	return r;
}
EXPORT_SYMBOL(chrome_bo_unpin);

/*For BO domain changes*/
/*GTT BO(write-combine), unbind to system memory(cached) */
/*can accel CPU access*/
int chrome_object_set_domain(struct chrome_object *bo,
		u32 domain)
{
	int r;

	r = ttm_bo_reserve(&bo->bo, true, false, false, 0);
	if (unlikely(r!=0))
		return r;

	via_ttm_placement_from_domain(bo, domain);

	r = ttm_bo_validate(&bo->bo, &bo->placement, false, false, false);

	if (unlikely(r != 0))
		DRM_ERROR( "chrome set domain= %x failed\n",domain);
	ttm_bo_unreserve(&bo->bo);
	return r;
}
/*GPU wait CPU*/
int chrome_object_wait_cpu_access(struct chrome_object *bo,
		struct drm_file *file_priv)
{
	int ret = 0;

	if(unlikely((atomic_read(&bo->bo.cpu_writers) > 0))){
		if (bo->owner_file){
			if (file_priv != bo->owner_file)
				ret = ttm_bo_wait_cpu(&bo->bo, false);
			else
				BUG_ON(file_priv == bo->owner_file);
		}
	}
	return ret;
}

