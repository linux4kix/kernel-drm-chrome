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
#ifndef _CHROME_DRV_H_
#define _CHROME_DRV_H_

#include "drm_sman.h"
#include "chrome_verifier.h"
#include "ttm/ttm_bo_api.h"
#include "ttm/ttm_bo_driver.h"
#include "ttm/ttm_placement.h"
#include "ttm/ttm_module.h"
#include "ttm/ttm_memory.h"

#define DRIVER_AUTHOR	"Various"

#define DRIVER_NAME		"chrome"
#define DRIVER_DESC		"CHROME Pro"
#define DRIVER_DATE		"20091016"

#define DRIVER_MAJOR		1
#define DRIVER_MINOR		1	
#define DRIVER_PATCHLEVEL	0

/* VT3353 chipset*/
#define VX800_FUNCTION3     0x3353
/* VT3409 chipset*/
#define VX855_FUNCTION3     0x3409
/* VT3410 chipset*/
#define VX900_FUNCTION3     0x3410

#define DRM_FILE_PAGE_OFFSET (0x100000000ULL >> PAGE_SHIFT)
#define Kb  (1024)
#define Mb  (Kb*Kb)
#define DEV_VQ_MEMORY (256 * 1024)
#define DEV_COMMAND_BUFFER_MEMORY (16*1024*1024)
#define DEV_PCIE_MEMORY (256*1024*1024UL)

struct chrome_object {
	struct ttm_buffer_object bo;
	struct ttm_bo_kmap_obj kmap;
	void *ptr;
	bool iomem;
	u32 placements[3];
	u32 busy_placements[3];
	struct ttm_placement placement;
	struct ttm_placement busy_placement;
	struct drm_file *owner_file; /*who grab this BO*/
	struct drm_file *reserved_by;
	unsigned long flags;
};

struct drm_chrome_shadow_map {
	struct drm_local_map  *shadow;
	unsigned int   shadow_size;
	unsigned int   *shadow_handle;
};

struct acpi_backup{
	unsigned char gti_backup[13];
	struct chrome_object *agp_gart_shadow;
};

struct chrome_cmdbuffer_ops {
	void (*kickoff_dma_ring)(struct drm_device *dev);
	int (*execute_branch_buffer)(struct drm_device *dev, 
		struct chrome_object *vbo, uint32_t cmd_size);
};
struct chrome_engine_ops {
	/* flush command ops */
	struct chrome_cmdbuffer_ops cmdbuffer_ops;
};
struct drm_chrome_private {
	struct ttm_bo_global_ref        bo_global_ref;
	struct ttm_global_reference	mem_global_ref;
	struct ttm_bo_device bdev;
	struct drm_device *ddev;

	uint64_t vram_start;	/* vram physical bus address */
	uint64_t vram_size;	/* M byte */
	int vram_mtrr;
	uint64_t ring_buffer_size;	/* Byte */
	uint64_t pcie_mem_size;	/* Byte */
	uint64_t pcie_gart_start;	/* offset from vram */

	u8 __iomem *mmio_map;
	u32 *pcie_gart_map;	/* kernel virtual address for accessing gart */
	/* kernel virtual address for accessing ringbuffer */
	u32 *pcie_ringbuffer_map;
	bool need_dma32;
	struct chrome_object *vq;
	struct chrome_object *agp_gart;
	struct chrome_object *agp_ringbuffer;
	/* a pointer to manage the AGP ring buffer */
	struct drm_chrome_dma_manager *dma_manager;
	struct chrome_fence_pool *p_fence; /* fence stuff */
	struct chrome_sg_move_manager *sg_manager;
	/*for shadow & event tag*/
	struct drm_chrome_shadow_map shadow_map;
	struct drm_clb_event_tag_info *event_tag_info;
#if CHROME_VERIFY_ENABLE
	struct drm_chrome_state hc_state;
#endif
	int *verify_buff;
	/*save GTI registers here when suspend*/
	struct acpi_backup pm_backup;
	/* via graphic chip ops */
	struct chrome_engine_ops  engine_ops;
	/*FIXME, A patch before using KMS
	 * gart may overwrite by other memory manager(vesa fb driver)
	 * gart_valid is false means gart-table may destoryed
	 * we should restore gart table from 
	 * gart shadow buffer
	 */
	bool gart_valid;

};


enum chrome_family {
	CHROME_OTHER = 0,	/* Baseline */
	CHROME_PRO_GROUP_A,/* Another video engine and DMA commands */
	CHROME_DX9_0,
	CHROME_PCIE_GROUP
};

/* CHROME MMIO register access */
#define CHROME_READ(reg)		\
	ioread32((u32 *)(p_priv->mmio_map + (reg)))
#define CHROME_WRITE(reg, val)	\
	iowrite32(val, (u32 *)(p_priv->mmio_map + (reg)))
#define CHROME_READ8(reg)		\
	ioread8(p_priv->mmio_map + (reg))
#define CHROME_WRITE8(reg, val)	\
	iowrite8(val, p_priv->mmio_map + (reg))
#endif
