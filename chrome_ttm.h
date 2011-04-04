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
#ifndef _CHROME_TTM_H_
#define _CHROME_TTM_H_
#define DMA_DEBUG 1

extern int
chrome_ttm_global_init(struct drm_chrome_private *dev_priv);
extern int chrome_init_ttm(struct drm_device *dev);
extern void chrome_ttm_fini(struct drm_device *dev);
extern struct ttm_backend *
chrome_pcie_backend_init(struct drm_chrome_private *p_priv);
extern int chrome_bo_move(struct ttm_buffer_object *bo,
				bool evict, bool interruptible,  bool no_wait_reserve,
			bool no_wait_gpu, struct ttm_mem_reg *new_mem);
extern int chrome_sg_move_init(struct drm_device *dev);
extern int chrome_sg_move_fini(struct drm_device *dev);

struct chrome_sg_info {
	struct chrome_descriptor **chrome_desc;
	int num_desc;
	int desc_per_page;
	uint32_t * dma_cmd_tmp;
	enum dma_data_direction direction;
	uint64_t chain_start;
};

struct chrome_sg_obj {
	struct chrome_sg_info chrome_sg_info;
	struct list_head ddestory;
	spinlock_t lock;
	struct chrome_fence_object *p_fence;
	struct drm_chrome_private *dev_priv;
};

struct chrome_sg_move_manager{	
	struct list_head ddestory;
	struct delayed_work sg_wq;
	spinlock_t lock;
	struct drm_chrome_private *dev_priv;
	
	struct chrome_object *sg_fence[4]; /*DMA fence src*/
	uint32_t *sg_fence_vaddr[4]; /*virtual address of fence in VRAM*/
	/*dest of fence blit by DMA*/
	uint32_t *sg_sync_vaddr; /*virtual address*/
	uint32_t *sg_sync_vaddr_align; /*aligned to 16, HW limitation*/
	dma_addr_t  sg_sync_bus_addr; /*BUS address, used by DAM engine*/
	uint64_t  sg_sync_bus_addr_align;
	bool initialized;
};

struct chrome_descriptor {
	uint32_t mem_addr_l;
	uint32_t mem_addr_h;
	uint32_t dev_addr;
	uint32_t size;
	uint32_t tile_mode;
	uint32_t next_l;
	uint32_t next_h;
	/*Chrome9_descriptor needs 16 byte align, use this field to fill*/
	uint32_t null;
};


#endif
