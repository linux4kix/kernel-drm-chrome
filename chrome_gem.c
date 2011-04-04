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
#include "chrome_object.h"
#include "chrome_gem.h"

int chrome_gem_object_create(struct drm_device *dev, int size,
			     int alignment, int initial_domain,
			     bool kernel,
			     bool interruptible,
			     struct drm_gem_object **obj)
{
	struct drm_chrome_private *dev_priv =
		(struct drm_chrome_private *)dev->dev_private;
	struct drm_gem_object *gobj;
	struct chrome_object *vobj;
	enum ttm_bo_type type = ttm_bo_type_device;
	int ret;

	size = roundup(size, PAGE_SIZE);
	if(kernel)
		type = ttm_bo_type_kernel;

	gobj = drm_gem_object_alloc(dev, size);
	if(gobj == NULL) {
		return -ENOMEM;
	}
	ret = chrome_buffer_object_create(&dev_priv->bdev, size, type,
			initial_domain, 0, 0, interruptible, NULL, &vobj);
	if (ret) {
		DRM_ERROR("Failed to allocate GEM object \n");
		mutex_lock(&dev->struct_mutex);
		drm_gem_object_unreference(gobj);
		mutex_unlock(&dev->struct_mutex);
		return ret;
	}
	gobj->driver_private = vobj;
	*obj = gobj;

	return 0;
}