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
#include "chrome9_dmablit.h"
#include <ttm/ttm_bo_api.h>
#include <ttm/ttm_bo_driver.h>
#include <ttm/ttm_placement.h>
#include <ttm/ttm_module.h>

struct chrome_ttm_backend {
	struct ttm_backend	backend; /* the class's method */
	struct drm_chrome_private	*p_priv; /* hook driver informaiton */
	unsigned long		num_pages;
	/* series of pages' data structure for binding */
	struct page		**pages;
	struct page		*dummy_read_page;
	/* indicate that the pages are populated */
	bool			populated;
	/* indicated whether the pages have been bound */
	bool			bound;
	/* GART start entry to which the pages bind */
	unsigned long		start_entry;
};



static int chrome_pcie_populate(struct ttm_backend *backend,
			    unsigned long num_pages, struct page **pages,
			    struct page *dummy_read_page)
{
	struct chrome_ttm_backend *p_backend;

	p_backend = container_of(backend,
		struct chrome_ttm_backend, backend);

	p_backend->populated = true;
	p_backend->pages = pages;
	p_backend->num_pages = num_pages;
	p_backend->dummy_read_page = dummy_read_page;

	return 0;
}

/* The reversion of function chrome_pcie_populate */
static void chrome_pcie_clear(struct ttm_backend *backend)
{
	struct chrome_ttm_backend *p_backend;

	p_backend = container_of(backend,
		struct chrome_ttm_backend, backend);
	p_backend->bound = false;
	p_backend->populated = false;
	p_backend->num_pages = 0;
	p_backend->pages = 0;
	p_backend->dummy_read_page = 0;
}

/* Update GART table relevant entry based on the parameters */
static int chrome_pcie_bind(struct ttm_backend *backend,
	struct ttm_mem_reg *bo_mem)
{
	struct chrome_ttm_backend *p_backend;
	struct drm_chrome_private *p_priv;
	unsigned long entry;
	u32 max_entries, i;
	u8 sr6c, sr6f;

	p_backend = container_of(backend,
		struct chrome_ttm_backend, backend);
	p_priv = p_backend->p_priv;

	if (!p_priv || !backend) {
		DRM_ERROR("Pass a invalid value into chrome_pcie_bind.\n");
		return -EINVAL;
	}

	/* bo_mem->mm_node record the mem space of pcie
	 * whose memory manager is based on page_size unit
	 */
	entry = bo_mem->mm_node->start;
	max_entries = p_priv->pcie_mem_size >> PAGE_SHIFT;

	/* sanity check */
	if (!p_backend->populated || p_backend->bound)
		DRM_ERROR("TTM bind a un-populated ttm_backend or an \
		already bound ttm_backend.\n");

	if (entry + p_backend->num_pages > max_entries) {
		DRM_ERROR("bind a page range exceeding the PCIE space.\n");
		return -EINVAL;
	}

	/* begin to bind pcie memory
	 * 1.disable gart table HW protect
	 * 2.update the relevant entries
	 * 3.invalide GTI cache
	 * 4.enable gart table HW protect
	 */
	/* 1.*/
	CHROME_WRITE8(0x83c4, 0x6c);
	sr6c = CHROME_READ8(0x83c5);
	sr6c &= 0x7F;
	CHROME_WRITE8(0x83c5, sr6c);
	/* 2.*/
	for (i = 0; i < p_backend->num_pages; i++) {
		writel(page_to_pfn(p_backend->pages[i]) & 0x3FFFFFFF,
			p_priv->pcie_gart_map + entry + i);
	}
	/* 3.*/
	CHROME_WRITE8(0x83c4, 0x6f);
	sr6f = CHROME_READ8(0x83c5);
	sr6f |= 0x80;
	CHROME_WRITE8(0x83c5, sr6f);
	/* 4.*/
	CHROME_WRITE8(0x83c4, 0x6c);
	sr6c = CHROME_READ8(0x83c5);
	sr6c |= 0x80;
	CHROME_WRITE8(0x83c5, sr6c);

	/* update the flag */
	p_backend->bound = true;
	p_backend->start_entry = entry;

	return 0;
	
}

static int chrome_pcie_unbind(struct ttm_backend *backend)
{
	struct chrome_ttm_backend *p_backend;
	struct drm_chrome_private *p_priv;
	unsigned long i;
	u8 sr6c, sr6f;

	p_backend = container_of(backend,
		struct chrome_ttm_backend, backend);
	p_priv = p_backend->p_priv;

	if (!backend || !p_priv) {
		DRM_ERROR("unbind function called with invalid parameters.\n");
		return -EINVAL;
	}

	if (!p_backend->bound || !p_backend->populated) {
		DRM_ERROR("unbind a ttm_backend which hasn't bound yet.\n");
		return -EINVAL;
	}

	/* begin to unbind pcie memory
	 * 1. disable gart table HW protect
	 * 2. unbind relevant entries
	 * 3. invalide GTI cache
	 * 4. enable gart table HW protect
	 */
	/* 1.*/
	CHROME_WRITE8(0x83c4, 0x6c);
	sr6c = CHROME_READ8(0x83c5);
	sr6c &= 0x7F;
	CHROME_WRITE8(0x83c5, sr6c);
	/* 2.*/
	for (i = 0; i < p_backend->num_pages; i++) {
		writel(0x80000000,
			p_priv->pcie_gart_map + p_backend->start_entry + i);
	}
	/* 3.*/
	CHROME_WRITE8(0x83c4, 0x6f);
	sr6f = CHROME_READ8(0x83c5);
	sr6f |= 0x80;
	CHROME_WRITE8(0x83c5, sr6f);
	/* 4.*/
	CHROME_WRITE8(0x83c4, 0x6c);
	sr6c = CHROME_READ8(0x83c5);
	sr6c |= 0x80;
	CHROME_WRITE8(0x83c5, sr6c);

	/* update some flags */
	p_backend->bound = false;
	p_backend->start_entry = -1;

	return 0;	
}

/* unbind pcie memory first if it is bound, free the memory */
static void chrome_pcie_destroy(struct ttm_backend *backend)
{
	struct chrome_ttm_backend *p_backend;

	p_backend = container_of(backend,
		struct chrome_ttm_backend, backend);

	if (p_backend->bound)
		chrome_pcie_unbind(backend);

	kfree(p_backend);
	p_backend = NULL;
}

static struct ttm_backend_func chrome_pcie_func = {
	.populate = chrome_pcie_populate,
	.clear = chrome_pcie_clear,
	.bind = chrome_pcie_bind,
	.unbind = chrome_pcie_unbind,
	.destroy = chrome_pcie_destroy,
};

/* allocate memory for struct chrome_ttm_backend and do intialization */
struct ttm_backend *
chrome_pcie_backend_init(struct drm_chrome_private *p_priv)
{
	struct chrome_ttm_backend *p_backend;

	if (!p_priv)
		return 0;

	p_backend = kzalloc(sizeof(struct chrome_ttm_backend), GFP_KERNEL);
	if (!p_backend)
		return 0;

	p_backend->backend.bdev = &p_priv->bdev;
	p_backend->backend.flags = 0;
	p_backend->backend.func = &chrome_pcie_func;
	p_backend->bound = false;
	p_backend->populated = false;
	p_backend->p_priv = p_priv;
	p_backend->dummy_read_page = 0;
	p_backend->num_pages = 0;
	p_backend->pages = 0;

	return &p_backend->backend;
}
