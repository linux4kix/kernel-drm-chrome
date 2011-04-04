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
#include "chrome9_dma.h"
#include "chrome_mm.h"
#include "chrome9_3d_reg.h"
#include "chrome_init.h"
#include "chrome_object.h"

#define RING_BUFFER_INIT_FLAG 1
#define RING_BUFFER_CLEANUP_FLAG 2

int chrome_modeset = 0;

MODULE_PARM_DESC(modeset, "Disable/Enable modesetting");
module_param_named(modeset, chrome_modeset, int, 0400);

int chrome_drm_get_pci_id(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	unsigned int *reg_val = data;
	outl(0x8000002C, 0xCF8);
	*reg_val = inl(0xCFC);
	outl(0x8000012C, 0xCF8);
	*(reg_val+1) = inl(0xCFC);
	outl(0x8000022C, 0xCF8);
	*(reg_val+2) = inl(0xCFC);
	outl(0x8000052C, 0xCF8);
	*(reg_val+3) = inl(0xCFC);

    return 0;
}

int chrome_dma_init(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	
	return 0;
}

struct drm_ioctl_desc chrome_ioctls[] = {
	DRM_IOCTL_DEF(DRM_CHROME_GEM_CREATE,
		chrome_ioctl_gem_create, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_MMAP,
		chrome_ioctl_gem_mmap, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_PREAD,
		chrome_ioctl_gem_pread, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_PWRITE,
		chrome_ioctl_gem_pwrite, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_SET_DOMAIN,
		chrome_ioctl_gem_set_domain, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_FLUSH,
		chrome_ioctl_gem_flush, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_WAIT,
		chrome_ioctl_gem_wait, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_SHADOW_INIT,
		chrome_ioctl_shadow_init, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_ALLOCATE_EVENT_TAG,
		chrome_ioctl_alloc_event, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_FREE_EVENT_TAG,
		chrome_ioctl_free_event, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_WAIT_CHIP_IDLE,
		chrome_ioctl_wait_chip_idle, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_CPU_GRAB,
		chrome_ioctl_cpu_grab, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_CPU_RELEASE,
		chrome_ioctl_cpu_release, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_PIN,
		chrome_ioctl_gem_pin, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_UNPIN,
		chrome_ioctl_gem_unpin, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_LEAVE,
		chrome_ioctl_leave_gem, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_CHROME_GEM_ENTER,
		chrome_ioctl_enter_gem, DRM_AUTH)
};

int chrome_max_ioctl = DRM_ARRAY_SIZE(chrome_ioctls);

static struct pci_device_id pciidlist[] = {
	{0x1106, 0x1122, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHROME_PCIE_GROUP},
	{0x1106, 0x5122, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHROME_PCIE_GROUP},
	{0x1106, 0x7122, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHROME_PCIE_GROUP},
	{0, 0, 0}
};

static struct drm_driver driver = {
	.driver_features =
		DRIVER_HAVE_DMA | DRIVER_FB_DMA | DRIVER_USE_MTRR | DRIVER_GEM,
	.load = chrome_driver_load,
	.preclose = chrome_preclose,
	.lastclose = chrome_lastclose,
	.unload = chrome_driver_unload,
	.suspend = chrome_drm_suspend,
	.resume = chrome_drm_resume,
	.reclaim_buffers = drm_core_reclaim_buffers,
	.get_reg_ofs = drm_core_get_reg_ofs,
	.get_map_ofs = drm_core_get_map_ofs,
	.gem_init_object = chrome_gem_object_init,
	.gem_free_object = chrome_gem_object_free,
	.ioctls = chrome_ioctls,
	.fops = {
		 .owner = THIS_MODULE,
		 .open = drm_open,
		 .release = drm_release,
		 .unlocked_ioctl = drm_ioctl,
		 .mmap = chrome_mmap,
		 .poll = drm_poll,
		 .fasync = drm_fasync,
	},
	.pci_driver = {
		 .name = DRIVER_NAME,
		 .id_table = pciidlist,
	},

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

static int __init chrome_init(void)
{
	driver.num_ioctls = chrome_max_ioctl;
	if (chrome_modeset)
		driver.driver_features |= DRIVER_MODESET;
	return drm_init(&driver);
}

static void __exit chrome_exit(void)
{
	drm_exit(&driver);
}

module_init(chrome_init);
module_exit(chrome_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");
