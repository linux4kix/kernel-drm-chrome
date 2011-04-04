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
#include "via_chrome9_drv.h"
#include "via_chrome9_drm.h"
#include "via_chrome9_object.h"
#include "via_chrome9_reloc.h"
#include "via_chrome9_fence.h"
#include "via_chrome9_3d_reg.h"
#include "via_chrome9_dma.h"
/**
 * modify the command buffer with reloc type
 */
static int 
via_chrome9_write_reloc(unsigned int *buffer_ptr, unsigned long offset,
                                            unsigned long valid_bo_offset, uint32_t delta,
                                            enum drm_via_chrome9_reloc_type type)
{
	unsigned int *cmdbuf = buffer_ptr + offset;
	unsigned long target_bo_offset = valid_bo_offset;
	unsigned int subaddr, value;

	switch(type) {
	case VIA_RELOC_2D:
		value = (target_bo_offset + delta) >> 3;
		writel(value, buffer_ptr + offset);
		break;
	case VIA_RELOC_3D:
		subaddr = (*cmdbuf) & 0xff000000;
		value = subaddr | ((target_bo_offset + delta) >> 8);
		writel(value, buffer_ptr + offset);
		break;
	case VIA_RELOC_VIDEO_HQV_SF:
		*cmdbuf = (target_bo_offset + delta)|0x1; /* local system dynamic buffer */
		break;
	case VIA_RELOC_VIDEO_SL:
		*cmdbuf = (target_bo_offset + delta);
		break;
	case VIA_RELOC_VERTEX_STREAM_L:
		subaddr = (*cmdbuf)&0xfff00fff;
		*cmdbuf = (subaddr) | ((((target_bo_offset + delta)&0x000003FF)>>2)<<12);
		break;
	case VIA_RELOC_VERTEX_STREAM_H:
		subaddr = (*cmdbuf)&0x000003FF;
		*cmdbuf = (subaddr) |  (((target_bo_offset + delta)>>10)<<10);
		break;
	default:
		DRM_INFO("undefine type for relocation \n");
		break;
	}

	return 0;
}
/* Try to parse Command type
 * Only handle 2d/3d command type
 * */
static void via_chrome9_parse_cmd_type(struct drm_device *dev,
	enum drm_via_chrome9_reloc_type type)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	struct drm_via_chrome9_dma_manager *cmd_manager = dev_priv->dma_manager;

	switch(type) {
		case VIA_RELOC_2D:
			cmd_manager->cmd_type |= VIA_CHROME9_CMD_2D;
			break;
		case VIA_RELOC_3D:
		case VIA_RELOC_VERTEX_STREAM_L:
		case VIA_RELOC_VERTEX_STREAM_H:
			cmd_manager->cmd_type |= VIA_CHROME9_CMD_3D;
			break;
		case VIA_RELOC_VIDEO_HQV_SF:
		case VIA_RELOC_VIDEO_SL:
			break;
	}
	return;
}
/**
 * valid the target bo
 */
int
via_chrome9_reloc_valid(struct drm_via_chrome9_gem_flush_parse *parse,
			unsigned long *pcmddata, int i)
{
	struct drm_via_chrome9_gem_relocation_entry *entry;
	unsigned int *command_buffer_ptr;
	int ret;

	entry = (struct drm_via_chrome9_gem_relocation_entry *)&parse->relocs[i];
	command_buffer_ptr = (unsigned int *)pcmddata;

	ret = via_chrome9_write_reloc(command_buffer_ptr, entry->offset,
				parse->reloc_buffer[i].vobj->bo.offset,
				entry->delta, entry->type);
	
	return ret;
}

/**
 * parse the command buffer
 */
int
via_chrome9_parse_init(struct drm_device *dev, struct drm_file *file_priv,
				struct drm_via_chrome9_gem_flush_parse *parse,
				struct drm_via_chrome9_gem_flush *data)
{
	INIT_LIST_HEAD(&parse->valid_list);

	parse->exec_objects = kzalloc(sizeof(struct drm_via_chrome9_gem_exec_object),
				GFP_KERNEL);
	if (parse->exec_objects == NULL) {
		DRM_ERROR("kzalloc the exec object error \n");
		return -ENOMEM;
	}

	if (DRM_COPY_FROM_USER(parse->exec_objects,
				(void __user*)(unsigned long)data->buffer_ptr,
				sizeof(struct drm_via_chrome9_gem_exec_object))) {
		return -EFAULT;
	}

	return 0;
}

static int 
via_chrome9_validate_init(struct drm_device *dev, struct drm_file *file_priv,
		struct drm_via_chrome9_gem_flush_parse *parse)
{	
	int i, ret = 0;
	struct drm_via_chrome9_gem_relocation_entry *entry;

	if (NULL == parse->relocs ||NULL == parse->fence)
		return -1;
	/* Traversing the reloc list and reserve BOs */
	for (i = 0; i < parse->exec_objects->relocation_count; i++) {
		entry =
		(struct drm_via_chrome9_gem_relocation_entry *)&parse->relocs[i];
		parse->reloc_buffer[i].reloc_count = i;
		parse->reloc_buffer[i].gobj = drm_gem_object_lookup(dev, file_priv,
						entry->target_handle);
		if(!parse->reloc_buffer[i].gobj)
			BUG();
		parse->reloc_buffer[i].vobj = parse->reloc_buffer[i].gobj->driver_private;
		/*If BO not been reserved, reserve it*/
		if (parse->reloc_buffer[i].vobj->reserved_by == NULL) {

			ret = ttm_bo_reserve(&parse->reloc_buffer[i].vobj->bo,
					true, false, false, 0);

			parse->reloc_buffer[i].vobj->reserved_by = file_priv;

			if (unlikely(ret != 0)) {
				DRM_ERROR("failed to reserve object.\n");
				return ret;
			}
			via_chrome9_parse_cmd_type(dev, parse->relocs[i].type);
		}
	}
	return ret;
}

static int 
via_chrome9_validate_bo(struct drm_device *dev, struct drm_file *file_priv,
		struct drm_via_chrome9_gem_flush_parse *parse)
{
	int i, ret = 0;
	struct via_chrome9_fence_object *old_fence = NULL;
	struct drm_via_chrome9_gem_relocation_entry *entry;

	for (i = 0; i < parse->exec_objects->relocation_count; i++) {
		entry =
		(struct drm_via_chrome9_gem_relocation_entry *)&parse->relocs[i];

		/*GPU wait CPU access here*/
		via_chrome9_object_wait_cpu_access(parse->reloc_buffer[i].vobj,file_priv);

		/*If domain not match, needs reloc*/
		if (!((parse->reloc_buffer[i].vobj->bo.mem.placement & entry->location_mask) &
					TTM_PL_MASK_MEM)){

			via_ttm_placement_from_domain(parse->reloc_buffer[i].vobj,
				entry->location_mask);
retry:
			/* validate the ttm buffer with proposed placement */
			ret = ttm_bo_validate(&parse->reloc_buffer[i].vobj->bo,
				       &parse->reloc_buffer[i].vobj->placement,
				       true, false, false);
			if (unlikely(ret)) {
				if (ret !=  -ERESTARTSYS) {
					DRM_ERROR("Failed to Move BO to location=%x in command flush,"
							"memory may not enough, pid=%d\n",
							entry->location_mask, current->pid);
					return ret;
				}else
					goto retry;
			}
		}
		/* add the fence to this bo */
		if (parse->fence && (parse->reloc_buffer[i].vobj->bo.sync_obj != parse->fence)) {
			old_fence = (struct via_chrome9_fence_object *)
					parse->reloc_buffer[i].vobj->bo.sync_obj;
			parse->reloc_buffer[i].vobj->bo.sync_obj =
					via_chrome9_fence_ref(parse->fence);
			parse->reloc_buffer[i].vobj->bo.sync_obj_arg = NULL;
		}
		/*GPU fetch command in serial sequence, so GPU no need to wait GPU idle*/
		/*BUT should wait BO Moving if using DMA */
		if (old_fence) {
			if (test_bit(TTM_BO_PRIV_FLAG_MOVING,
				&parse->reloc_buffer[i].vobj->bo.priv_flags))
				if (!via_chrome9_fence_wait(old_fence, NULL,
					false, false))
					clear_bit(TTM_BO_PRIV_FLAG_MOVING,
					&parse->reloc_buffer[i].vobj->bo.priv_flags);
			via_chrome9_fence_unref(&old_fence);
		}

		/* compare the bo offset and add it to valid list if mismatch*/
		if (parse->reloc_buffer[i].vobj->bo.offset != entry->presumed_offset) {
			parse->need_correct = true;
			entry->presumed_offset = parse->reloc_buffer[i].vobj->bo.offset;
			INIT_LIST_HEAD(&parse->reloc_buffer[i].list);
			list_add_tail(&parse->reloc_buffer[i].list, &parse->valid_list);
		}
	}
	return ret;
}

static void 
via_chrome9_validate_fini(struct drm_device *dev, struct drm_file *file_priv,
				struct drm_via_chrome9_gem_flush_parse *parse)
{
	int i;
	if (!parse->reloc_buffer)
		return;

	for (i = 0; i < parse->exec_objects->relocation_count; i++) {
		if (parse->reloc_buffer[i].vobj && 
			parse->reloc_buffer[i].vobj->reserved_by == file_priv) {
			ttm_bo_unreserve(&parse->reloc_buffer[i].vobj->bo);
			parse->reloc_buffer[i].vobj->reserved_by = NULL;
		}
	}
}
/**
 * parse the reloc list and add the bo to valid list
 */
int
via_chrome9_parse_reloc(struct drm_device *dev, struct drm_file *file_priv,
				struct drm_via_chrome9_gem_flush_parse *parse)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	int  ret;

	parse->need_correct = false;

	ret = via_chrome9_fence_object_create(dev_priv, &parse->fence, false);
	if (ret) {
		DRM_ERROR(" can not allocate fence object for flush \n");
		return -ENOMEM;
	}

	if (!parse->exec_objects->relocation_count)
		return 0;
	/* allocate the space for relocs and reloc buffer, copy the data from user space */
	parse->relocs = kzalloc(parse->exec_objects->relocation_count *
			sizeof(struct drm_via_chrome9_gem_relocation_entry),
			GFP_KERNEL);
	if (parse->relocs == NULL) {
		DRM_ERROR("kzalloc the relocs error \n");
		return -ENOMEM;
	}

	parse->reloc_buffer = kzalloc(parse->exec_objects->relocation_count *
			sizeof(struct via_chrome9_flush_buffer), GFP_KERNEL);
	if (parse->reloc_buffer == NULL) {
		DRM_ERROR("kzalloc the reloc_buffer error \n");
		return -ENOMEM;
	}

	if (DRM_COPY_FROM_USER(parse->relocs,
			(void __user*)(unsigned long)parse->exec_objects->relocs_ptr,
			parse->exec_objects->relocation_count *
			sizeof(struct drm_via_chrome9_gem_relocation_entry))) {
		DRM_ERROR("DRM_COPY_FROM_USER relocs error \n");
		return -EFAULT;
	}

	/*validate BOs */
	if ((ret = via_chrome9_validate_init(dev, file_priv, parse))){
		via_chrome9_validate_fini(dev, file_priv, parse);
		return ret;
	}
	if ((ret = via_chrome9_validate_bo(dev, file_priv, parse))){
		via_chrome9_validate_fini(dev, file_priv, parse);
		send_sig(SIGTERM, current, 1);
		return ret;
	}

	/* update the buffer object offset to user space */
	if (parse->need_correct) {
		if (DRM_COPY_TO_USER(
			(void __user*)(unsigned long)parse->exec_objects->relocs_ptr,
			parse->relocs,
			parse->exec_objects->relocation_count *
			sizeof(struct drm_via_chrome9_gem_relocation_entry))) {

			DRM_ERROR("DRM_COPY_TO_USER relocs error \n");
			return -EFAULT;
		}
	}

	return 0;
}

/**
 * the fini for this exec
 */
void
via_chrome9_parse_fini(struct drm_device *dev, struct drm_file *file_priv,
				struct drm_via_chrome9_gem_flush_parse *parse,
				int error)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct via_chrome9_fence_object *old_fence = NULL;
	int i, ret;

	if (parse->fence) {
		if (!error) {
			ret = via_chrome9_fence_emit(dev_priv, parse->fence);
			if (ret) {
				DRM_ERROR(" can not emit fence object \n");
			}
		}
		/* unreference this fence for future destroy */
		via_chrome9_fence_unref(&parse->fence);
	}
	via_chrome9_validate_fini(dev, file_priv, parse);

	for (i = 0; i < parse->exec_objects->relocation_count; i++) {
		if (parse->reloc_buffer[i].gobj) {
			if (error) {
				old_fence = (struct via_chrome9_fence_object *)
					parse->reloc_buffer[i].vobj->bo.sync_obj;
					parse->reloc_buffer[i].vobj->bo.sync_obj = NULL;
				if (old_fence) {
					via_chrome9_fence_unref(&old_fence);
				}
			}
			mutex_lock(&dev->struct_mutex);
			drm_gem_object_unreference(parse->reloc_buffer[i].gobj);
			mutex_unlock(&dev->struct_mutex);
		}
	}

	kfree(parse->exec_objects);
	kfree(parse->relocs);
	kfree(parse->reloc_buffer);
	parse->exec_objects = NULL;
	parse->relocs = NULL;
	parse->reloc_buffer = NULL;

}
