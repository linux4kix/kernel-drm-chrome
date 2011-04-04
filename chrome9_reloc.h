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
#ifndef _CHROME_RELOC_H_
#define _CHROME_RELOC_H_

struct chrome_flush_buffer {
	/* the list for valid */
	struct list_head list;
	/* the gem object for this bo */
	struct drm_gem_object *gobj;
	/* the via chrome9 object to this bo */
	struct chrome_object *vobj;
	/* the bo location count at reloc list */
	unsigned int reloc_count;
};

struct drm_chrome_gem_flush_parse {
	/* the list for valid */
	struct list_head	valid_list;
	/* the exec object for this emit */
	struct drm_chrome_gem_exec_object *exec_objects;
	/* the reloc list */
	struct drm_chrome_gem_relocation_entry *relocs;
	/* the bo at reloc list */
	struct chrome_flush_buffer *reloc_buffer;
	/* the fence for flush */
	struct chrome_fence_object *fence;
	/* the mask for us valid bo */
	bool need_correct;
};

extern int
chrome_parse_init(struct drm_device *dev, struct drm_file *file_priv,
				struct drm_chrome_gem_flush_parse *parse,
				struct drm_chrome_gem_flush *data);
extern int
chrome_parse_reloc(struct drm_device *dev, struct drm_file *file_priv,
				struct drm_chrome_gem_flush_parse *parse);
extern int
chrome_reloc_valid(struct drm_chrome_gem_flush_parse *parse,
				unsigned long *pcmddata, int i);
extern void
chrome_parse_fini(struct drm_device *dev, struct drm_file *file_priv,
				struct drm_chrome_gem_flush_parse *parse,
				int error);
#endif