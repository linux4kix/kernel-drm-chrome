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
#include "drmP.h"
#include "chrome_drm.h"
#include "chrome_drv.h"
#include "chrome_mm.h"
#include "chrome9_dma.h"
#include "chrome_object.h"
#include "chrome9_3d_reg.h"
#include "chrome_gem.h"
#include "chrome9_reloc.h"

int chrome_ioctl_gem_create(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_gem_create *args = data;
	struct drm_gem_object *gobj;
	struct chrome_object *vobj;
	uint32_t handle;
	int ret;

	ret = chrome_gem_object_create(dev, args->size, args->alignment,
				     args->initial_domain, false, true, &gobj);
	if (ret) {
		DRM_ERROR("Failed to allocate a gem object ");
		return ret;
	}
	ret = drm_gem_handle_create(file_priv, gobj, &handle);
	if (ret) {
		mutex_lock(&dev->struct_mutex);
		drm_gem_object_unreference(gobj);
		mutex_unlock(&dev->struct_mutex);
		return ret;
	}
	mutex_lock(&dev->struct_mutex);
	drm_gem_object_handle_unreference(gobj);
	mutex_unlock(&dev->struct_mutex);
	vobj = (struct chrome_object *)gobj->driver_private;
	args->handle = handle;
	args->offset = vobj->bo.offset; 

	return 0;
}

int chrome_ioctl_gem_mmap(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_gem_mmap *args = data;
	struct drm_gem_object *gobj;
	struct chrome_object *vobj;
	int ret;
        
	gobj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (gobj == NULL) {
		return -EINVAL;
	}
	vobj = (struct chrome_object *)gobj->driver_private;
	ret = chrome_object_mmap(file_priv, vobj, args->size, &args->addr_ptr,
								&args->virtual);
	mutex_lock(&dev->struct_mutex);
	drm_gem_object_unreference(gobj);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int chrome_ioctl_gem_pread(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	return -1;
}

int chrome_ioctl_gem_pwrite(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	return -1;
}

int chrome_ioctl_gem_set_domain(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_gem_set_domain *args = data;
	struct drm_gem_object *gobj;
	struct chrome_object *vobj;
	uint32_t domain = args->write_domain;

	int ret;

	gobj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (gobj == NULL) {
		return -EINVAL;
	}
	vobj = gobj->driver_private;
	if (domain == CHROME_GEM_DOMAIN_CPU)
		chrome_object_wait(vobj, false);
	ret= chrome_object_set_domain(vobj, domain);
	mutex_lock(&dev->struct_mutex);
	drm_gem_object_unreference(gobj);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int chrome_ioctl_gem_wait(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_gem_wait *args = data;
	struct drm_gem_object *gobj;
	struct chrome_object *vobj;
	bool no_wait;
	int ret;

	gobj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (gobj == NULL) {
		return -EINVAL;
	}
	no_wait = (args->no_wait != 0);
	vobj = gobj->driver_private;
	ret = chrome_object_wait(vobj, no_wait);
	mutex_lock(&dev->struct_mutex);
	drm_gem_object_unreference(gobj);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int chrome_ioctl_gem_flush(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{	
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *) dev->dev_private;
	struct drm_chrome_dma_manager *lpcmdmamanager =
		(struct drm_chrome_dma_manager *) dev_priv->dma_manager;
	struct drm_chrome_gem_flush *args = data;
	struct drm_chrome_gem_flush_parse parse; 
	struct drm_gem_object *gobj;
	struct chrome_object *cmd_vobj;
	int ret;

	memset(&parse, 0, sizeof(struct drm_chrome_gem_flush_parse));

	mutex_lock(&lpcmdmamanager->command_flush_lock);
	ret = chrome_parse_init(dev, file_priv, &parse, args);
	if (ret) {
		DRM_ERROR(" can not parse the exec buffer \n");
		chrome_parse_fini(dev, file_priv, &parse, ret);
		mutex_unlock(&lpcmdmamanager->command_flush_lock);
		return -EFAULT;
	}

	ret = chrome_parse_reloc(dev, file_priv, &parse);
	if (ret) {
		DRM_ERROR(" can not parse the reloc \n");
		chrome_parse_fini(dev, file_priv, &parse, ret);
		mutex_unlock(&lpcmdmamanager->command_flush_lock);
		return -EFAULT;
	}
	if (args->flag & CHROME_FLUSH_BRANCH_BUFFER) {
		/* use branch buffer*/
		gobj = drm_gem_object_lookup(dev, file_priv, parse.exec_objects->cmd_bo_handle);
		if (!gobj) {
			drm_gem_object_unreference_unlocked(gobj);
			mutex_unlock(&lpcmdmamanager->command_flush_lock);
			return -EINVAL;
		}
		cmd_vobj = gobj->driver_private;

		ret = chrome_branchbuffer_flush(dev, cmd_vobj,
			parse.exec_objects->buffer_length, &parse);
		drm_gem_object_unreference_unlocked(gobj);
	} else 
		ret = chrome_ringbuffer_flush(dev,
					(unsigned int *)(unsigned long)parse.exec_objects->buffer_ptr,
					parse.exec_objects->buffer_length, true, &parse);
	if (ret) {
		DRM_ERROR(" can not flush the command \n");
		chrome_parse_fini(dev, file_priv, &parse, ret);
		mutex_unlock(&lpcmdmamanager->command_flush_lock);
		return -EFAULT;
	}

	chrome_parse_fini(dev, file_priv, &parse, ret);
	mutex_unlock(&lpcmdmamanager->command_flush_lock);

	return 0;
}

int chrome_ioctl_cpu_grab(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_cpu_grab *args = data;
	struct drm_gem_object *gobj;
	struct chrome_object *vobj;
	int ret = 0;

	gobj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (gobj == NULL) {
		ret =  -EINVAL;
		goto out;
	}
	vobj = gobj->driver_private;

	if (vobj->owner_file) {
		/*process already grab this BO*/
		if (vobj->owner_file == file_priv)
			goto out;
		else { /*other process grab it, wait */
			if ((ret = ttm_bo_wait_cpu(&vobj->bo, false)))
				 goto out;
		}
	}
	/*If no process grab it, wait Idle & grab it*/
	if (!(ret= ttm_bo_synccpu_write_grab(&vobj->bo, false)))
		vobj->owner_file = file_priv;

out:
	drm_gem_object_unreference_unlocked(gobj);
	return ret;

}


int chrome_ioctl_cpu_release(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_cpu_release *args = data;
	struct drm_gem_object *gobj;
	struct chrome_object *vobj;
	int ret = 0;

	gobj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (gobj == NULL) {
		ret =  -EINVAL;
		goto out;
	}
	vobj = gobj->driver_private;

	if (vobj->owner_file) {
		/*If this process grab this BO, release it*/
		if (vobj->owner_file == file_priv) {
			ttm_bo_synccpu_write_release(&vobj->bo);
			vobj->owner_file = NULL;
		}
	}

out:
	drm_gem_object_unreference_unlocked(gobj);
	return ret;
}

int chrome_ioctl_gem_pin(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_gem_pin *args = data;
	struct drm_gem_object *gobj;
	struct chrome_object *vobj;
	int ret;

	/* Now only support bo pinned in the vram */
	if (!(args->location_mask & CHROME_GEM_DOMAIN_VRAM)) {
		DRM_ERROR("chrome_ioctl_gem_pin passed in error memory \
			type 0x%x", args->location_mask);
		return -EINVAL;
	}

	gobj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (!gobj) {
		drm_gem_object_unreference_unlocked(gobj);
		return -EINVAL;
		
	}
	vobj = gobj->driver_private;

	ret = chrome_bo_pin(vobj, args->location_mask, &args->offset);
	drm_gem_object_unreference_unlocked(gobj);

	return ret;
}

int chrome_ioctl_gem_unpin(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_gem_unpin *args = data;
	struct drm_gem_object *gobj;
	struct chrome_object *vobj;
	int ret;

	gobj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (gobj == NULL) {
		drm_gem_object_unreference_unlocked(gobj);
		return -EINVAL;
	}
	vobj = gobj->driver_private;

	ret = chrome_bo_unpin(vobj);
	drm_gem_object_unreference_unlocked(gobj);

	return ret;
}

/*Init Event tag*/
int chrome_ioctl_shadow_init(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_shadow_init *init = data;
	struct drm_clb_event_tag_info *event_tag_info;
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;

	if (init->shadow_size) {
		/* find apg shadow region mappings */
	dev_priv->shadow_map.shadow = drm_core_findmap(dev, init->shadow_handle);
	if (!dev_priv->shadow_map.shadow) {
		DRM_ERROR("could not find shadow map!\n");
		goto error;
	}
	dev_priv->shadow_map.shadow_size = init->shadow_size;
	dev_priv->shadow_map.shadow_handle = 
		(unsigned int *)dev_priv->shadow_map.shadow->handle;
	}

	event_tag_info = vmalloc(sizeof(struct drm_clb_event_tag_info));
	if (!event_tag_info)
		return DRM_ERROR(" event_tag_info allocate error!");
	memset(event_tag_info, 0, sizeof(struct drm_clb_event_tag_info));

	event_tag_info->linear_address = dev_priv->shadow_map.shadow_handle;
	event_tag_info->event_tag_linear_address = event_tag_info->linear_address + 3;
	dev_priv->event_tag_info =  event_tag_info;

	return 0;
error:
	return -1;
}

int chrome_ioctl_shadow_fini(struct drm_device *dev)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;
	if (!dev_priv->event_tag_info) {
		vfree(dev_priv->event_tag_info);
		dev_priv->event_tag_info = NULL;
	}
	return 0;
}
/*Allocate event tag for 3D*/
int chrome_ioctl_alloc_event(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_event_tag *event_tag = data;
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;
	struct drm_clb_event_tag_info *event_tag_info =
		dev_priv->event_tag_info;
	unsigned int *event_addr = 0, i = 0;

	for (i = 0; i < NUMBER_OF_EVENT_TAGS; i++) {
		if (!event_tag_info->usage[i])
			break;
	}

	if (i < NUMBER_OF_EVENT_TAGS) {
		event_tag_info->usage[i] = 1;
		event_tag->event_offset = i;
		event_tag->last_sent_event_value.event_low = 0;
		event_tag->current_event_value.event_low = 0;
		event_addr = event_tag_info->linear_address +
		event_tag->event_offset * 4;
		*event_addr = 0;
		return 0;
	} else {
		return -7;
	}

	return 0;
}
/*Free event tag for 3D*/
int chrome_ioctl_free_event(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;
	struct drm_clb_event_tag_info *event_tag_info =
		dev_priv->event_tag_info;
	struct drm_chrome_event_tag *event_tag = data;

	event_tag_info->usage[event_tag->event_offset] = 0;
	return 0;
}

static int
waitchipidle_inv(struct drm_chrome_private *dev_priv)
{
	unsigned int count = 50000;
	unsigned int eng_status;
	unsigned int engine_busy;

	do {
		eng_status =
			getmmioregister(dev_priv->mmio_map,
					INV_RB_ENG_STATUS);
		engine_busy = eng_status & INV_ENG_BUSY_ALL;
		count--;
	}
	while (engine_busy && count)
		;
	if (count && engine_busy == 0)
		return 0;
	return -1;
}
int
chrome_ioctl_wait_chip_idle(struct drm_device *dev, void *data,
			 struct drm_file *file_priv)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *) dev->dev_private;

	waitchipidle_inv(dev_priv);
	/* maybe_bug here, do we always return 0 */
	return 0;
}

/*
 * save Bos by evict them to system memory
 */
void chrome_drm_save_bos(struct drm_device *dev)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;

	/*evict VRAM BO*/
	chrome_evict_vram(dev);
	/*backup gart-table*/
	memcpy(dev_priv->pm_backup.agp_gart_shadow->ptr,
		dev_priv->agp_gart->ptr, dev_priv->pcie_mem_size >> 10);
	/* FIXME, patch before using KMS
	 * set gart table invalid*/
	dev_priv->gart_valid = false;
	return;
}
/*
 * Restore BOs
 * need do nothing except restore gart-table 
 */
void chrome_drm_restore_bos(struct drm_device *dev)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;
	/*FIXME*/
	if (!dev_priv->gart_valid) {
		/*needs restore gart-table*/
		memcpy(dev_priv->agp_gart->ptr,
			dev_priv->pm_backup.agp_gart_shadow->ptr,
			dev_priv->pcie_mem_size >> 10);
	}
	dev_priv->gart_valid = true;
	return;
}

int  chrome_drm_resume(struct drm_device *dev)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;
	unsigned int i;

	pci_set_power_state(dev->pdev, PCI_D0);
	pci_restore_state(dev->pdev);
	if (pci_enable_device(dev->pdev))
		return -1;
	pci_set_master(dev->pdev);

	setmmioregister(dev_priv->mmio_map, INV_REG_CR_TRANS, 0x00110000);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN,
		0x06000000);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN,
		0x07100000);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_TRANS,
		INV_PARATYPE_PRECR);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN,
		INV_SUBA_HSETRBGID | INV_HSETRBGID_CR);

	/* Here restore SR66~SR6F SR79~SR7B */
	for (i = 0; i < 10; i++) {
		setmmioregisteru8(dev_priv->mmio_map,
			0x83c4, 0x66 + i);
		setmmioregisteru8(dev_priv->mmio_map,
			0x83c5, dev_priv->pm_backup.gti_backup[i]);
	}

	for (i = 0; i < 3; i++) {
		setmmioregisteru8(dev_priv->mmio_map,
			0x83c4, 0x79 + i);
		setmmioregisteru8(dev_priv->mmio_map,
		 0x83c5, dev_priv->pm_backup.gti_backup[10 + i]);
	}

	/*enable HW interrupt*/

	/* FIXME
	 * Do bo restore in EnterVT now,
	 * needs if we using KMS
	 */
#if 0
	chrome_drm_restore_bos(dev);

	/*re-init gart-table & ringbuffer*/
	chrome_init_gart_table(dev);
	set_agp_ring_cmd_inv(dev);
#endif
	return 0;
}

int  chrome_drm_suspend(struct drm_device *dev,
	pm_message_t state)
{
	int i;
	unsigned char reg_tmp;
	struct pci_dev *pci = dev->pdev;
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;
#if 0
	struct drm_chrome_private *p_priv = dev_priv;
	u8 sr6c;
#endif
	pci_save_state(pci);

	/* Save registers from SR66~SR6F */
	for (i = 0; i < 10; i++) {
		setmmioregisteru8(dev_priv->mmio_map, 0x83c4, 0x66 + i);
		dev_priv->pm_backup.gti_backup[i] =
			getmmioregisteru8(dev_priv->mmio_map, 0x83c5);
	}

	/* Save registers from SR79~SR7B */
	for (i = 0; i < 3; i++) {
		setmmioregisteru8(dev_priv->mmio_map, 0x83c4, 0x79 + i);
		dev_priv->pm_backup.gti_backup[10 + i] =
			getmmioregisteru8(dev_priv->mmio_map, 0x83c5);
	}

	/* Patch for PCIE series S3 hang issue inspired by BIOS team:
	 * Close IGA channel and clear FIFO depth
	 */
	/* Disable first display channel */
	setmmioregisteru8(dev_priv->mmio_map, 0x83d4, 0x17);
	reg_tmp = getmmioregisteru8(dev_priv->mmio_map, 0x83d5);
	setmmioregisteru8(dev_priv->mmio_map, 0x83d5,
		reg_tmp & 0x7f);

	/* Disable second display channel */
	setmmioregisteru8(dev_priv->mmio_map, 0x83d4, 0x6A);
	reg_tmp = getmmioregisteru8(dev_priv->mmio_map, 0x83d5);
	setmmioregisteru8(dev_priv->mmio_map, 0x83d5,
		reg_tmp & 0x7f);

	/* First screen off */
	setmmioregisteru8(dev_priv->mmio_map, 0x83c4, 0x01);
	reg_tmp = getmmioregisteru8(dev_priv->mmio_map, 0x83c5);
	setmmioregisteru8(dev_priv->mmio_map, 0x83c5,
		reg_tmp | 0x20);

	/* Second screen off */
	setmmioregisteru8(dev_priv->mmio_map, 0x83d4, 0x6B);
	reg_tmp = getmmioregisteru8(dev_priv->mmio_map, 0x83d5);
	setmmioregisteru8(dev_priv->mmio_map, 0x83d5,
		(reg_tmp | 0x04) & (~0x02));

	/* Clear IGA1 FIFO depth
	 * 3C5.17[7:0]=0
	 */
	setmmioregisteru8(dev_priv->mmio_map, 0x83c4, 0x17);
	setmmioregisteru8(dev_priv->mmio_map, 0x83c5, 0x0);

	/* Clear IGA2 FIFO depth
	 * 3x5.68[7:4]=0, 3x5.94[7]=0, 3x5.95[7]=0
	 */
	setmmioregisteru8(dev_priv->mmio_map, 0x83d4, 0x68);
	reg_tmp = getmmioregisteru8(dev_priv->mmio_map, 0x83d5);
	setmmioregisteru8(dev_priv->mmio_map, 0x83d5,
		reg_tmp & 0x0f);

	setmmioregisteru8(dev_priv->mmio_map, 0x83d4, 0x94);
	reg_tmp = getmmioregisteru8(dev_priv->mmio_map, 0x83d5);
	setmmioregisteru8(dev_priv->mmio_map, 0x83d5,
		reg_tmp & 0x7f);

	setmmioregisteru8(dev_priv->mmio_map, 0x83d4, 0x95);
	reg_tmp = getmmioregisteru8(dev_priv->mmio_map, 0x83d5);
	setmmioregisteru8(dev_priv->mmio_map, 0x83d5,
		reg_tmp & 0x7f);

	/* save bos, FIXME
	 * do this in LeaveVT now,
	 * but needs do here when using KMS
	 */
#if 0
 	chrome_drm_save_bos(dev);

	/* enable gti write */
	CHROME_WRITE8(0x83c4, 0x6c);
	sr6c = CHROME_READ8(0x83c5);
	sr6c &= 0x7F;
	CHROME_WRITE8(0x83c5, sr6c);

	/*disable HW interrupt*/

	/*LeaveVT will wait Engine Idle, should we need to wait ringbuffer stop here?*/

#endif
	/*stop ringbuffer*/
	set_agp_ring_buffer_stop(dev);
/* patch for ACPI S1 */
#if 0
	if (state.event == PM_EVENT_SUSPEND) {
			/* Shut down the device */
			pci_disable_device(dev->pdev);
			pci_set_power_state(dev->pdev, PCI_D3hot);
	}
#endif

	return 0;
}

static int
chrome_gem_release_cpu(int id, void *ptr, void *data)
{
	struct drm_gem_object *gobj = ptr;
	struct chrome_object *vobj = gobj->driver_private;

	if (vobj->owner_file) {
		/*If this process grab this BO, release it*/
		if (vobj->owner_file == data) {
			ttm_bo_synccpu_write_release(&vobj->bo);
			vobj->owner_file = NULL;
		}
	}
	return 0;
}

void chrome_release_cpu_grab(struct drm_device *dev,
		struct drm_file *file_priv)
{
	/*release cpu grab of this process*/
	idr_for_each(&file_priv->object_idr,
	     &chrome_gem_release_cpu, file_priv);
}

/* 
 * Called before enter another memory manager, which will allcate 
 * video memory by it self(e.g vesa fb driver),
 * so we must save video memory BOs.
 * */
int chrome_ioctl_leave_gem(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_chrome_private *p_priv =
		(struct drm_chrome_private *)dev->dev_private;
	u8 sr6c;

	chrome_drm_save_bos(dev);

	/* enable gti write */
	CHROME_WRITE8(0x83c4, 0x6c);
	sr6c = CHROME_READ8(0x83c5);
	sr6c &= 0x7F;
	CHROME_WRITE8(0x83c5, sr6c);

	return 0;
}

/*
 *  Called when backto GEM memory manager
 *  we need do some restore here
 */
int chrome_ioctl_enter_gem(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	chrome_drm_restore_bos(dev);
	/*re-init gart-table & ringbuffer*/
	chrome_init_gart_table(dev);
	set_agp_ring_cmd_inv(dev);
	return 0;
}

void chrome_preclose(struct drm_device *dev, struct drm_file *file_priv)
{
	chrome_release_cpu_grab(dev,file_priv);
}
void chrome_lastclose(struct drm_device *dev)
{
	chrome_ioctl_shadow_fini(dev);
}
