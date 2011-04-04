/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
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
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "drmP.h"
#include "via_drv.h"
#include "drm_pciids.h"

int via_modeset = 0;

MODULE_PARM_DESC(modeset, "Disable/Enable modesetting");
module_param_named(modeset, via_modeset, int, 0400);

static struct pci_device_id via_pci_table[] __devinitdata = {
	viadrv_PCI_IDS,
};
MODULE_DEVICE_TABLE(pci, via_pci_table);

static struct drm_driver via_driver;

#define VIA_AGP_MODE_MASK	0x17
#define VIA_AGPV3_MODE		0x08
#define VIA_AGPV3_8X_MODE	0x02
#define VIA_AGPV3_4X_MODE	0x01
#define VIA_AGP_4X_MODE		0x04
#define VIA_AGP_2X_MODE		0x02
#define VIA_AGP_1X_MODE		0x01
#define VIA_AGP_FW_MODE		0x10

static int via_detect_agp(struct drm_device *dev)
{
	struct drm_via_private *dev_priv = dev->dev_private;
	struct drm_agp_info agp_info;
	struct drm_agp_mode mode;
	int ret = 0;

	ret = drm_agp_acquire(dev);
	if (ret) {
		DRM_ERROR("Failed acquiring AGP device.\n");
		return ret;
	}

	ret = drm_agp_info(dev, &agp_info);
	if (ret) {
		DRM_ERROR("Failed detecting AGP aperture size.\n");
		goto out_err0;
	}

	mode.mode = agp_info.mode & ~VIA_AGP_MODE_MASK;
	if (mode.mode & VIA_AGPV3_MODE)
		mode.mode |= VIA_AGPV3_8X_MODE;
	else
		mode.mode |= VIA_AGP_4X_MODE;

	mode.mode |= VIA_AGP_FW_MODE;
	ret = drm_agp_enable(dev, mode);
	if (ret) {
		DRM_ERROR("Failed to enable the AGP bus.\n");
		goto out_err0;
	}

	ret = ttm_bo_init_mm(&dev_priv->bdev, TTM_PL_TT, agp_info.aperture_size >> PAGE_SHIFT);
	if (!ret) {
		DRM_INFO("Detected %lu MB of AGP Aperture at "
			"physical address 0x%08lx.\n",
			agp_info.aperture_size >> 20,
			agp_info.aperture_base);
	} else {
out_err0:
		drm_agp_release(dev);
	}
        return ret;
}

#define VIA_MMIO_REGSIZE 0x9000

static int via_mmio_setup(struct drm_device *dev)
{
	struct drm_via_private *dev_priv = dev->dev_private;
	int ret, len = pci_resource_len(dev->pdev, 1);
	struct ttm_buffer_object *bo;

	ret = ttm_bo_init_mm(&dev_priv->bdev, TTM_PL_PRIV0,
				len >> PAGE_SHIFT);
	if (ret)
		return ret;

	ret = ttm_bo_allocate(&dev_priv->bdev, VIA_MMIO_REGSIZE, ttm_bo_type_kernel,
				TTM_PL_FLAG_PRIV0, 1, PAGE_SIZE,
				0, false, via_ttm_bo_destroy,
				NULL, &bo);
	if (ret)
		goto err;

	ret = ttm_bo_reserve(bo, true, false, false, 0);
	if (ret)
		goto err;

	ret = ttm_bo_kmap(bo, 0, bo->num_pages, &dev_priv->mmio);
	ttm_bo_unreserve(bo);
err:
	if (!ret) {
		DRM_INFO("Detected MMIO at physical address 0x%08llx.\n",
			(unsigned long long) pci_resource_start(dev->pdev, 1));
		dev_priv->iga[0].vga_regs = (dev_priv->mmio.virtual + 0x83C0);
		dev_priv->iga[1].vga_regs = (dev_priv->mmio.virtual + 0x83C0);
	} else
		ttm_bo_clean_mm(&dev_priv->bdev, TTM_PL_PRIV0);
	return ret;
}

static int via_driver_unload(struct drm_device *dev)
{
	struct drm_via_private *dev_priv = dev->dev_private;
	struct ttm_buffer_object *bo = dev_priv->mmio.bo;
	int ret = 0;

	ret = via_dma_cleanup(dev);
	if (ret)
		return ret;

	if (bo) {
		ret = ttm_bo_reserve(bo, true, false, false, 0);
		if (!ret)
			ttm_bo_kunmap(&dev_priv->mmio);
		ttm_bo_unreserve(bo);
		ttm_bo_unref(&bo);
	}

	ttm_bo_clean_mm(&dev_priv->bdev, TTM_PL_PRIV0);
	ttm_bo_clean_mm(&dev_priv->bdev, TTM_PL_VRAM);
	ttm_bo_clean_mm(&dev_priv->bdev, TTM_PL_TT);

	ttm_global_fini(&dev_priv->mem_global_ref,
			&dev_priv->bo_global_ref,
			&dev_priv->bdev);

	if (dev->agp && dev->agp->acquired)
		drm_agp_release(dev);

	kfree(dev_priv);
	return ret;
}

static int via_driver_load(struct drm_device *dev, unsigned long chipset)
{
	struct drm_via_private *dev_priv;
	int ret = 0;

	dev_priv = kzalloc(sizeof(struct drm_via_private), GFP_KERNEL);
	if (dev_priv == NULL)
		return -ENOMEM;

	dev->dev_private = (void *)dev_priv;
	dev_priv->chipset = chipset;
	dev_priv->dev = dev;

	ret = via_ttm_init(dev_priv);
	if (ret)
		goto out_err;

	ret = via_mmio_setup(dev);
	if (ret) {
		DRM_INFO("VIA MMIO region failed to map\n");
		goto out_err;
	}

	ret = via_detect_vram(dev);
	if (ret)
		goto out_err;

	if (dev->agp && drm_pci_device_is_agp(dev)) {
		ret = via_detect_agp(dev);
		if (ret)
			goto out_err;
	}

	if (dev->driver->driver_features & DRIVER_MODESET) {
		ret = via_modeset_init(dev);
		if (ret)
			goto out_err;
	}

	ret = drm_vblank_init(dev, 1);
out_err:
	if (ret)
		via_driver_unload(dev);
	return ret;
}

static int via_final_context(struct drm_device *dev, int context)
{
	struct drm_via_private *dev_priv = dev->dev_private;

	via_release_futex(dev_priv, context);

	/* Linux specific until context tracking code gets ported to BSD */
	/* Last context, perform cleanup */
	if (dev->ctx_count == 1 && dev->dev_private) {
		DRM_DEBUG("Last Context\n");
		drm_irq_uninstall(dev);
		via_cleanup_futex(dev_priv);
		via_dma_cleanup(dev);
	}
	return 1;
}

static void via_lastclose(struct drm_device *dev)
{
	struct drm_via_private *dev_priv = dev->dev_private;

	if (!dev_priv)
		return;
}

static void via_reclaim_buffers_locked(struct drm_device *dev,
                                struct drm_file *file_priv)
{
	mutex_lock(&dev->struct_mutex);

	if (dev->driver->dma_quiescent)
		dev->driver->dma_quiescent(dev);

	mutex_unlock(&dev->struct_mutex);
	return;
}

static struct drm_driver via_driver = {
	.driver_features =
		DRIVER_USE_AGP | DRIVER_USE_MTRR | DRIVER_HAVE_IRQ |
		DRIVER_GEM | DRIVER_IRQ_SHARED,
	.load = via_driver_load,
	.unload = via_driver_unload,
	.context_dtor = via_final_context,
	.get_vblank_counter = via_get_vblank_counter,
	.enable_vblank = via_enable_vblank,
	.disable_vblank = via_disable_vblank,
	.irq_preinstall = via_driver_irq_preinstall,
	.irq_postinstall = via_driver_irq_postinstall,
	.irq_uninstall = via_driver_irq_uninstall,
	.irq_handler = via_driver_irq_handler,
	.dma_quiescent = via_driver_dma_quiescent,
	.reclaim_buffers = drm_core_reclaim_buffers,
	.reclaim_buffers_locked = NULL,
	.reclaim_buffers_idlelocked = via_reclaim_buffers_locked,
	.lastclose = via_lastclose,
	.gem_init_object = via_gem_init_object,
	.ioctls = via_ioctls,
	.fops = {
		.owner = THIS_MODULE,
		.open = drm_open,
		.release = drm_release,
		.unlocked_ioctl = drm_ioctl,
		.mmap = drm_gem_mmap,
		.poll = drm_poll,
		.fasync = drm_fasync,
		.llseek = noop_llseek,
	},

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

static int __devinit
via_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	return drm_get_pci_dev(pdev, ent, &via_driver);
}

static void __devexit
via_pci_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);

	drm_put_dev(dev);
}

#ifdef CONFIG_PM
int via_pci_suspend(struct pci_dev *pdev, pm_message_t state)
{
	return 0;
}

int via_pci_resume(struct pci_dev *pdev)
{
	return 0;
}
#endif

static struct pci_driver via_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = via_pci_table,
	.probe	= via_pci_probe,
	.remove	= __devexit_p(via_pci_remove),
#ifdef CONFIG_PM
	.suspend = via_pci_suspend,
	.resume = via_pci_resume,
#endif
};

static int __init via_init(void)
{
	via_driver.num_ioctls = via_max_ioctl;
	via_init_command_verifier();

	if (via_modeset)
		via_driver.driver_features |= DRIVER_MODESET;
	return drm_pci_init(&via_driver, &via_pci_driver);
}

static void __exit via_exit(void)
{
	drm_pci_exit(&via_driver, &via_pci_driver);
}

module_init(via_init);
module_exit(via_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");
