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
#ifndef _CHROME_FENCE_H_
#define _CHROME_FENCE_H_

#include <asm/atomic.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/kref.h>
#include <ttm/ttm_bo_api.h>
#include "chrome_drv.h"

#define CHROME_FENCE_SYNC_BO_SIZE PAGE_SIZE
/*min DMA move size is 16 , and must be 16 align*/
#define CHROME_DMA_FENCE_SIZE 31

struct chrome_fence_object {
	/* point to chrome private data structure */
	struct drm_chrome_private *p_priv;
	/* the reference information of this fence object */
	struct kref kref;
	/* indicate which list this fence object belongs to */
	struct list_head list;
	/* the sequence number that the fence object emit */
	uint32_t	sequence;
	/* the time to wait for the fence object signal */
	unsigned long timeout;
	/* whether this fence object emit or not */
	bool emited;
	/* whether this fence object has been signaled */
	bool signaled;
	/*If a DMA fence or not*/
	bool is_dma;
};

struct chrome_fence_pool {
	struct chrome_object *fence_sync;
	uint32_t *fence_sync_virtual;
	uint64_t	fence_bus_addr;
	/* Fence command temp buffer */
	uint32_t *p_cmd_tmp;
	/* for access synchronization */
	spinlock_t fence_pool_lock;
	/* the fence sequence emitter */
	atomic_t	seq;
	atomic_t dma_seq;
	/* for processes which wait for specified fence object to sleep */
	wait_queue_head_t	fence_wait_queue;
	/* the list hook all the initially created fence objects */
	struct list_head		created;
	/* the list hook all the emitted fence objects */
	struct list_head		emited;
	/* the list hook all the signaled fence objects */
	struct list_head		signaled;
	/*For DMA fence*/
	struct list_head		dma_created;
	struct list_head		dma_emited;
	struct list_head		dma_signaled;	
};

extern int
chrome_fence_object_create(struct drm_chrome_private *p_priv,
		struct chrome_fence_object **pp_fence_object, bool is_dma);
extern int chrome_fence_emit(struct drm_chrome_private *p_priv,
		struct chrome_fence_object *p_fence_object);
extern bool chrome_fence_signaled(
	struct chrome_fence_object *p_fence, void *sync_arg);
extern int chrome_fence_wait(
	struct chrome_fence_object *p_fence,
	void *sync_arg, bool lazy, bool interruptible);
extern int chrome_fence_flush(
	struct chrome_fence_object *p_fence, void *sync_arg);
extern void chrome_fence_unref(
	struct chrome_fence_object **pp_fence);
extern void *chrome_fence_ref(
	struct chrome_fence_object *p_fence);
extern int chrome_fence_mechanism_init(struct drm_device *dev);
extern void chrome_fence_mechanism_fini(struct drm_device *dev);

#endif
