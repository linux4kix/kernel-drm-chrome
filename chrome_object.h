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
#ifndef _CHROME_OBJECT_H_
#define _CHROME_OBJECT_H_

#include "ttm/ttm_bo_api.h"
extern int chrome_allocate_basic_bo(struct drm_device *dev);

extern void via_ttm_placement_from_domain(struct chrome_object *vbo,
				u32 domain);

extern int chrome_buffer_object_create(struct ttm_bo_device *bdev,
				unsigned long size,
				enum ttm_bo_type type,
				uint32_t flags,
				uint32_t page_alignment,
				unsigned long buffer_start,
				bool interruptible,
				struct file *persistant_swap_storage,
				struct chrome_object **p_bo);
extern int chrome_buffer_object_create2(unsigned long size,
				enum ttm_bo_type type,
				uint32_t flags,
				uint32_t page_alignment,
				unsigned long buffer_start,
				bool interruptible,
				struct file *persistant_swap_storage,
				struct chrome_object **p_bo);
extern int
chrome_object_wait(struct chrome_object *vobj, bool no_wait);
extern int chrome_buffer_object_kmap(struct chrome_object *vobj,
				void **ptr);
extern int
chrome_gem_object_init(struct drm_gem_object *obj);
extern void
chrome_gem_object_free(struct drm_gem_object *gobj);
extern void
chrome_buffer_object_unref(struct chrome_object **vobj);

extern void
chrome_buffer_object_kunmap(struct chrome_object *vobj);

extern void
chrome_buffer_object_ref(struct ttm_buffer_object *bo);

extern int
chrome_object_mmap(struct drm_file *file_priv,
				struct chrome_object *vobj,
				uint64_t size, uint64_t *offset, void **virtual);
extern int
chrome_mmap(struct file *filp, struct vm_area_struct *vma);
int chrome_evict_vram(struct drm_device *dev);
int chrome_bo_pin(struct chrome_object *bo,
		u32 domain, u64 *gpu_addr);
int chrome_bo_unpin(struct chrome_object *bo);
int chrome_object_set_domain(struct chrome_object *bo,
		u32 domain);
int chrome_object_wait_cpu_access(struct chrome_object *bo,
		struct drm_file *file_priv);
#endif