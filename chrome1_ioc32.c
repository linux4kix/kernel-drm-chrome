/*
 * Copyright 2006 Tungsten Graphics Inc., Bismarck, ND., USA.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*
 * Authors: Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#include "drmP.h"
#include "chrome_drv.h"

static int chrome_agp_init(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	struct drm_chrome_private *dev_priv = dev->dev_private;
	drm_chrome_agp_t *agp = data;

	dev_priv->agp_offset = agp->offset;
	DRM_INFO("AGP offset = %x, size = %u MB\n", agp->offset, agp->size >> 20);
	return 0;
}

static int chrome_fb_init(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	struct drm_chrome_private *dev_priv = dev->dev_private;
	drm_chrome_fb_t *fb = data;

	dev_priv->vram_offset = fb->offset;
	DRM_INFO("FB offset = 0x%08x, size = %u MB\n", fb->offset, fb->size >> 20);
	return 0;
}

static int chrome_do_init_map(struct drm_device *dev, drm_chrome_init_t *init)
{
	struct drm_chrome_private *dev_priv = dev->dev_private;

	dev_priv->sarea = drm_getsarea(dev);
	if (!dev_priv->sarea) {
		DRM_ERROR("could not find sarea!\n");
		dev->dev_private = (void *)dev_priv;
		chrome_dma_cleanup(dev);
		return -EINVAL;
	}

	dev_priv->sarea_priv =
		(drm_chrome_sarea_t *) ((u8 *) dev_priv->sarea->handle +
					init->sarea_priv_offset);

	/**
	 * The UMS xorg servers depends on the virtual address which it 
	 * received by mmap in userland. This is for backward compatiablity
	 * only. Once we have full KMS x servers this will go away. 
	 */
	if (init->mmio_offset) {
		drm_local_map_t *tmp;

		if (dev_priv->mmio.bo) {
			ttm_bo_kunmap(&dev_priv->mmio);
			ttm_bo_unref(&dev_priv->mmio.bo);
			dev_priv->mmio.bo = NULL;
		}

		tmp = drm_core_findmap(dev, init->mmio_offset);
		dev_priv->mmio.virtual = tmp->handle;
		if (!dev_priv->mmio.virtual) {
			DRM_ERROR("could not find mmio region!\n");
			dev->dev_private = (void *)dev_priv;
			chrome_dma_cleanup(dev);
			return -EINVAL;
		}
	}

	chrome_init_futex(dev_priv);

	chrome_init_dmablit(dev);

	dev->dev_private = (void *)dev_priv;
	return 0;
}

static int chrome_map_init(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	drm_chrome_init_t *init = data;
	int ret = 0;

	switch (init->func) {
	case VIA_INIT_MAP:
		ret = chrome_do_init_map(dev, init);
		break;
	case VIA_CLEANUP_MAP:
		ret = chrome_dma_cleanup(dev);
		break;
	}
	return ret;
}

static int chrome_mem_alloc(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_chrome_private *dev_priv = dev->dev_private;
	int type = TTM_PL_FLAG_VRAM, start = dev_priv->vram_offset, ret = -EINVAL;
	struct ttm_buffer_object *bo = NULL;
	struct drm_gem_object *gem;
	drm_chrome_mem_t *mem = data;
	u32 handle = 0;

	if (mem->type > VIA_MEM_AGP) {
		DRM_ERROR("Unknown memory type allocation\n");
		return ret;
	}

	if (mem->type == VIA_MEM_AGP) {
		start = dev_priv->agp_offset; 
		type = TTM_PL_FLAG_TT;
	}

	gem = chrome_gem_create(dev, &dev_priv->bdev, type, PAGE_SIZE, start,
				mem->size);
	if (!gem)
		return ret;

	bo = gem->driver_private;
	if (bo) {
		ret = drm_gem_handle_create(file_priv, gem, &handle);
		drm_gem_object_unreference_unlocked(gem);
	}

	if (!ret) {
		mem->size = gem->size;
		mem->offset = bo->mem.start << PAGE_SHIFT;
		mem->index = (unsigned long) handle;
		ret = 0;
	} else {
		mem->offset = 0;
		mem->size = 0;
		if (gem)
			kfree(gem);
		DRM_DEBUG("Video memory allocation failed\n");
	}
	mem->index = (unsigned long) handle;
	DRM_INFO("offset %lu size %u index %lu\n", mem->offset, mem->size, mem->index);
	return ret;
}

static int chrome_mem_free(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	struct drm_gem_object *obj;
	drm_chrome_mem_t *mem = data;
	int ret = 0;

	/*
	 * Yes this is stolen from gem_close_ioctl. Just
	 * for old userland support. Will go away!!!
	 */
	spin_lock(&file_priv->table_lock);

	/* Check if we currently have a reference on the object */
	obj = idr_find(&file_priv->object_idr, mem->index);
	if (obj == NULL) {
		spin_unlock(&file_priv->table_lock);
		return -EINVAL;
	}

	/* Release reference and decrement refcount. */
	idr_remove(&file_priv->object_idr, mem->index);
	spin_unlock(&file_priv->table_lock);

	drm_gem_object_handle_unreference_unlocked(obj);
	DRM_DEBUG("free = 0x%lx\n", mem->index);
	return ret;
}

static int
chrome_gem_alloc(struct drm_device *dev, void *data,
		struct drm_file *file_priv)
{
	struct drm_chrome_private *dev_priv = dev->dev_private;
	struct ttm_buffer_object *bo = NULL;
	struct drm_gem_create *args = data;
	__u32 domain = args->read_domains;
	struct drm_gem_object *gem;
	int ret = -EINVAL;

	if (!domain)
		domain = args->write_domains;

	gem = chrome_gem_create(dev, &dev_priv->bdev, domain, PAGE_SIZE,
				0, args->size);
	if (!gem)
		return ret;
 
	bo = gem->driver_private;
	if (bo) {
		ret = drm_gem_handle_create(file_priv, gem, &args->handle);
		drm_gem_object_unreference_unlocked(gem);
		if (ret) {
			DRM_DEBUG("Video memory allocation failed\n");
			kfree(gem);
		}
	}
	return ret;
}

struct drm_ioctl_desc chrome_ioctls[] = {
	DRM_IOCTL_DEF_DRV(VIA_ALLOCMEM, chrome_mem_alloc, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_FREEMEM, chrome_mem_free, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_AGP_INIT, chrome_agp_init, DRM_AUTH|DRM_MASTER),
	DRM_IOCTL_DEF_DRV(VIA_FB_INIT, chrome_fb_init, DRM_AUTH|DRM_MASTER),
	DRM_IOCTL_DEF_DRV(VIA_MAP_INIT, chrome_map_init, DRM_AUTH|DRM_MASTER),
	DRM_IOCTL_DEF_DRV(VIA_DEC_FUTEX, chrome_decoder_futex, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_DMA_INIT, chrome_dma_init, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_CMDBUFFER, chrome_cmdbuffer, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_FLUSH, chrome_flush_ioctl, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_PCICMD, chrome_pci_cmdbuffer, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_CMDBUF_SIZE, chrome_cmdbuf_size, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_WAIT_IRQ, chrome_wait_irq, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_DMA_BLIT, chrome_dma_blit, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_BLIT_SYNC, chrome_dma_blit_sync, DRM_AUTH),
	DRM_IOCTL_DEF_DRV(VIA_GEM_CREATE, chrome_gem_alloc, DRM_AUTH)
};

int chrome_max_ioctl = DRM_ARRAY_SIZE(chrome_ioctls);
