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
#include <linux/seq_file.h>
#include <asm/atomic.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/kref.h>
#include "drmP.h"
#include "drm.h"
#include "chrome_fence.h"
#include "chrome_object.h"
#include "chrome9_3d_reg.h"
#include "chrome9_dma.h"
#include "chrome_ttm.h"
static void chrome_fence_object_destroy(struct kref *kref)
{
	struct chrome_fence_object *p_fence;

	p_fence = container_of(kref, struct chrome_fence_object, kref);
	spin_lock(&p_fence->p_priv->p_fence->fence_pool_lock);
	list_del(&p_fence->list);
	INIT_LIST_HEAD(&p_fence->list);
	spin_unlock(&p_fence->p_priv->p_fence->fence_pool_lock);

	kfree(p_fence);
	p_fence = NULL;
}

static bool
chrome_fence_poll_locked(struct chrome_fence_object *p_fence)
{
	uint32_t seq_queried;
	struct list_head *cur_p, *n, *pfence_emited, *pfence_signaled;
	struct drm_chrome_private *p_priv = p_fence->p_priv;
	struct chrome_fence_object *p_fence_obj;

	if (p_fence->is_dma) {
		pfence_emited = &p_priv->p_fence->dma_emited;
		pfence_signaled = &p_priv->p_fence->dma_signaled;
		seq_queried = readl(p_fence->p_priv->sg_manager->sg_sync_vaddr_align);
#if DMA_DEBUG
		printk(KERN_ALERT "chrome DMA debug: DMA fence poll fence =%d \n",seq_queried);
#endif

	}else{
		pfence_emited = &p_priv->p_fence->emited;
		pfence_signaled = &p_priv->p_fence->signaled;
		seq_queried = readl(p_priv->p_fence->fence_sync_virtual);
	}

	n = NULL;
	list_for_each(cur_p, pfence_emited) {
		p_fence_obj = container_of(cur_p,
			struct chrome_fence_object, list);
		if (p_fence_obj->sequence <= seq_queried)
			n = cur_p;
		else
			break;
	}

	if (n) {
		cur_p = n;
		while (cur_p != pfence_emited) {
			n = cur_p->prev;
			list_del(cur_p);
			list_add_tail(cur_p, pfence_signaled);
			p_fence_obj = container_of(cur_p,
				struct chrome_fence_object, list);
			p_fence_obj->signaled = true;
			cur_p = n;
		}
		return true;
	}

	return false;
}

/* The TTM bo driver specific function */
bool chrome_fence_signaled(struct chrome_fence_object *p_fence,
	void *sync_arg)
{

	if (!p_fence)
		return true;

	spin_lock(&p_fence->p_priv->p_fence->fence_pool_lock);
	if (p_fence->signaled || !p_fence->emited) {
		spin_unlock(&p_fence->p_priv->p_fence->fence_pool_lock);
		return true;
	}

	chrome_fence_poll_locked(p_fence);	
	spin_unlock(&p_fence->p_priv->p_fence->fence_pool_lock);

	return p_fence->signaled;
}

/* The TTM bo driver specific function */
int chrome_fence_wait(struct chrome_fence_object *p_fence,
	void *sync_arg,	bool lazy, bool interruptible)
{
	int ret;
	int timeout = HZ / 100;

	if (chrome_fence_signaled(p_fence, sync_arg))
		return 0;
retry:
	if (time_after(jiffies, p_fence->timeout)) {
		DRM_INFO("The fence wait timeout timeout = %u, jiffies \
			= %u.\n",
			(uint32_t)p_fence->timeout, (uint32_t)jiffies);
		return -EBUSY;
	}

	if (interruptible) {
		ret = wait_event_interruptible_timeout(
			p_fence->p_priv->p_fence->fence_wait_queue,
			chrome_fence_signaled(p_fence, sync_arg),
			timeout);
		if (ret < 0) {
			return ret;
		}
	} else
		ret = wait_event_timeout(
			p_fence->p_priv->p_fence->fence_wait_queue,
			chrome_fence_signaled(p_fence, sync_arg),
			timeout);
	if (unlikely(!chrome_fence_signaled(p_fence, sync_arg))) {
		goto retry;
	}

	if (unlikely(!(ret = chrome_fence_signaled(p_fence, sync_arg)))) {
		printk("fence wait function out with fence un-signaled.\n");
		return -EBUSY;
	}

	if (!ret) {
		printk("The fence wait timeout timeout = %u, jiffies\
			= %u.\n",
			(uint32_t)p_fence->timeout, (uint32_t)jiffies);
		/* need to reset the GPU ???*/ /* FIXME */
		return -EBUSY;
	}

	return 0;
}

/* The TTM bo driver specific function */
int chrome_fence_flush(struct chrome_fence_object *p_fence,
	void *sync_arg)
{
	return 0;
}

/* The TTM bo driver specific function */
void chrome_fence_unref(struct chrome_fence_object **pp_fence)
{
	struct chrome_fence_object *p_fence = *pp_fence;

	*pp_fence = NULL;
	if (p_fence)
		kref_put(&p_fence->kref, &chrome_fence_object_destroy);
}

/* The TTM bo driver specific function */
void *chrome_fence_ref(struct chrome_fence_object *p_fence)
{
	kref_get(&p_fence->kref);
	return p_fence;
}

static void chrome_blit(struct chrome_fence_object *p_fence_object)
{
	uint32_t *p_cmd = p_fence_object->p_priv->p_fence->p_cmd_tmp;
	struct drm_chrome_private *p_priv = p_fence_object->p_priv;
	unsigned int cmd_type = p_priv->dma_manager->cmd_type;
	unsigned int wait_flag = 0;

#ifndef USE_2D_ENGINE_BLT
	unsigned int  i, FenceCfg0, FenceCfg1, FenceCfg2, FenceCfg3;
	unsigned long fence_phy_address = 
		p_fence_object->p_priv->p_fence->fence_bus_addr;

	FenceCfg0 = (INV_SubA_HFCWBasH | (fence_phy_address & 0xFF000000)>>24);
	FenceCfg1 = (INV_SubA_HFCWBasL2 | (fence_phy_address & 0x00FFFFF0));
	FenceCfg2 = (INV_SubA_HFCIDL2 | (p_fence_object->sequence & 0x00FFFFFF));
	FenceCfg3 = (INV_SubA_HFCTrig2 | INV_HFCTrg | INV_HFCMode_SysWrite |
		INV_HFCMode_Idle | ((p_fence_object->sequence & 0xFF000000) >> 16));	

	/* set wait flag */
	if (cmd_type & CHROME_CMD_2D)
		wait_flag |= INV_CR_WAIT_2D_IDLE;
	if (cmd_type & CHROME_CMD_3D)
		wait_flag = INV_CR_WAIT_3D_IDLE;

	/* sync the engine*/
	addcmdheader0_invi(p_cmd, 1);
	add2dcmd_invi(p_cmd, 0x0000006C, wait_flag);
	addcmddata_invi(p_cmd, 0xCC000000);
	addcmddata_invi(p_cmd, 0xCC000000);

	/* use CR fence mechanism */
	addcmdheader2_invi(p_cmd, INV_REG_CR_TRANS,
			INV_ParaType_CR);
	addcmddata_invi(p_cmd, FenceCfg0);
	addcmddata_invi(p_cmd, FenceCfg1);
	addcmddata_invi(p_cmd, FenceCfg2);
	addcmddata_invi(p_cmd, FenceCfg3);
	for (i = 0 ; i < AGP_REQ_FIFO_DEPTH/2; i++) {
		addcmdheader2_invi(p_cmd, INV_REG_CR_TRANS,
			INV_ParaType_Dummy);
		addcmddata_invi(p_cmd, 0xCC0CCCCC);
	}
#else
	/* sync the 2D&3D engine*/
	addcmdheader0_invi(p_cmd, 1);
	add2dcmd_invi(p_cmd, 0x0000006C, 0x00400000|0x00800000);

	addcmdheader0_invi(p_cmd, 8);
	/* mode set to 32bpp */
	add2dcmd_invi(p_cmd, 0x04, 0x00000300);
	/* Disable color key */
	add2dcmd_invi(p_cmd, 0x48, 0);
	/* Dest base address */
	add2dcmd_invi(p_cmd, 0x14,
		p_fence_object->p_priv->p_fence->fence_sync->bo.offset >> 3);
	/* Assume pitch equal 4 * 8 == 16 byte */
	add2dcmd_invi(p_cmd, 0x08, 4 << 16);
	/* set destination position: assume x:0, y:0 */
	add2dcmd_invi(p_cmd, 0x10, 0 << 16 | 0);
	/* Set destination dimension 1 x 1 */
	add2dcmd_invi(p_cmd, 0x0c, 0 << 16 | 0);
	/* set fg color */
	add2dcmd_invi(p_cmd, 0x58, p_fence_object->sequence);
	/* set 2D command: blit + fix color pattarn + P rop */
	add2dcmd_invi(p_cmd, 0x00, 0xF0002001);
#endif	
	/* Do command size 128 bits alignment */
	if ((p_cmd - p_fence_object->p_priv->p_fence->p_cmd_tmp) & 0xF) {
		*(p_cmd)++ = 0xCC000000;
		*(p_cmd)++ = 0xCC000000;   
	}
	p_priv->dma_manager->cmd_type = 0;
	chrome_ringbuffer_flush(p_fence_object->p_priv->ddev,
		p_fence_object->p_priv->p_fence->p_cmd_tmp,
		p_cmd - p_fence_object->p_priv->p_fence->p_cmd_tmp,
		false, NULL);
}

/* the api function exposed for other parts */
int chrome_fence_emit(struct drm_chrome_private *p_priv,
		struct chrome_fence_object *p_fence_object)
{
	if (!p_fence_object) {
		printk("Invalid parameters passed in.\n");
		return -EINVAL;
	}

	spin_lock(&p_priv->p_fence->fence_pool_lock);
	if (p_fence_object->emited) {
		spin_unlock(&p_priv->p_fence->fence_pool_lock);
		return 0;
	}

	list_del(&p_fence_object->list);
	if (p_fence_object->is_dma)
		p_fence_object->sequence = atomic_add_return(1, &p_priv->p_fence->dma_seq);
	else
		p_fence_object->sequence = atomic_add_return(1, &p_priv->p_fence->seq);
	spin_unlock(&p_priv->p_fence->fence_pool_lock);

	/* blit out the sequence using 2D engine */
	if (!p_fence_object->is_dma)
		chrome_blit(p_fence_object) ;
    
	p_fence_object->emited = true;
	p_fence_object->timeout = jiffies + 6 * HZ;
    
	spin_lock(&p_priv->p_fence->fence_pool_lock);
	if (p_fence_object->is_dma) {
		list_add_tail(&p_fence_object->list, &p_priv->p_fence->dma_emited);
	}else
		list_add_tail(&p_fence_object->list, &p_priv->p_fence->emited);
	spin_unlock(&p_priv->p_fence->fence_pool_lock);

	return 0;
}

/* the api function exposed for other parts */
int
chrome_fence_object_create(struct drm_chrome_private *p_priv,
		struct chrome_fence_object **pp_fence_object, bool is_dma)
{
	struct chrome_fence_object *p_fence;

	p_fence = kzalloc(sizeof(struct chrome_fence_object), GFP_KERNEL);
	if (!p_fence) {
		printk(" fence object create out of memory.\n");
		return -ENOMEM;
	}

	p_fence->p_priv = p_priv;
	kref_init(&p_fence->kref);
	INIT_LIST_HEAD(&p_fence->list);
	p_fence->sequence = 0;
	p_fence->timeout = 0;
	p_fence->emited = false;
	p_fence->signaled = false;

	spin_lock(&p_priv->p_fence->fence_pool_lock);
	if (is_dma) {
		p_fence->is_dma = true;
		list_add_tail(&p_fence->list, &p_priv->p_fence->dma_created);
	}else
		list_add_tail(&p_fence->list, &p_priv->p_fence->created);
	spin_unlock(&p_priv->p_fence->fence_pool_lock);

	*pp_fence_object = p_fence;

	return 0;
}

int chrome_fence_mechanism_init(struct drm_device *dev)
{
	struct drm_chrome_private *p_priv =
		(struct drm_chrome_private *)dev->dev_private;
	int ret;

	if (unlikely(!p_priv || p_priv->p_fence)) {
		printk("fence mechanism called with invalid parameters or \
			the fence mechanism has been already initilized!\n");
		return -EINVAL;
	}

	p_priv->p_fence =
		kzalloc(sizeof(struct chrome_fence_pool), GFP_KERNEL);
	if (unlikely(!p_priv->p_fence)) {
		printk(" In fence mechanism init function, \
			not enough system memory.\n");
		return -ENOMEM;
	}

	/* allocate fence sync bo */
	ret = chrome_buffer_object_create(&p_priv->bdev,
				CHROME_FENCE_SYNC_BO_SIZE,
				ttm_bo_type_kernel,
#ifndef USE_2D_ENGINE_BLT
				TTM_PL_FLAG_TT| TTM_PL_FLAG_NO_EVICT,
#else
				TTM_PL_FLAG_VRAM | TTM_PL_FLAG_NO_EVICT,
#endif
				0, 0, false, NULL,
				&p_priv->p_fence->fence_sync);
	if (unlikely(ret)) {
		printk("allocate fence sync bo error.\n");
		goto out_err0;
	}

	ret = chrome_buffer_object_kmap(p_priv->p_fence->fence_sync,
				(void **)&p_priv->p_fence->fence_sync_virtual);
	if (unlikely(ret)) {
		printk("kmap fence sync bo error.\n");
		goto out_err1;
	}
#ifndef USE_2D_ENGINE_BLT
	p_priv->p_fence->fence_bus_addr = dma_map_page(
			&p_priv->ddev->pdev->dev,
			p_priv->p_fence->fence_sync->bo.ttm->pages[0], 0, 
			PAGE_SIZE, DMA_BIDIRECTIONAL);
#endif
#define FENCE_CMD_TMP_BUFFER (256 * sizeof(uint32_t))
	/* We assert 30 * sizeof(uint32_t) is enough for emit fence sequence */
	p_priv->p_fence->p_cmd_tmp = kzalloc(FENCE_CMD_TMP_BUFFER, GFP_KERNEL);
	if (!p_priv->p_fence->p_cmd_tmp) {
		printk("fence tmp cmd buffer allocate error. out of memory.\n");
		goto out_err2;
	}
#undef FENCE_CMD_TMP_BUFFER

	spin_lock_init(&p_priv->p_fence->fence_pool_lock);
	writel(0, p_priv->p_fence->fence_sync_virtual);
	atomic_set(&p_priv->p_fence->seq, 0);
	atomic_set(&p_priv->p_fence->dma_seq, 0);

	init_waitqueue_head(&p_priv->p_fence->fence_wait_queue);
	INIT_LIST_HEAD(&p_priv->p_fence->created);
	INIT_LIST_HEAD(&p_priv->p_fence->emited);
	INIT_LIST_HEAD(&p_priv->p_fence->signaled);
	INIT_LIST_HEAD(&p_priv->p_fence->dma_created);
	INIT_LIST_HEAD(&p_priv->p_fence->dma_emited);
	INIT_LIST_HEAD(&p_priv->p_fence->dma_signaled);	
	return 0;

	out_err2:
		chrome_buffer_object_kunmap(p_priv->p_fence->fence_sync);
	out_err1:
		chrome_buffer_object_unref(&p_priv->p_fence->fence_sync);
	out_err0:
		kfree(p_priv->p_fence);
		p_priv->p_fence = NULL;

	return ret;
}

void chrome_fence_mechanism_fini(struct drm_device *dev)
{
	struct drm_chrome_private *p_priv =
		(struct drm_chrome_private *)dev->dev_private;

	/* How about the waked up process to touch fence context?
	 * The context will be destroyed immediately in this function.
	 * FIXME.
	 */
	wake_up_all(&p_priv->p_fence->fence_wait_queue);
#ifndef USE_2D_ENGINE_BLT
	dma_unmap_page(	&p_priv->ddev->pdev->dev,p_priv->p_fence->fence_bus_addr,
			PAGE_SIZE, DMA_BIDIRECTIONAL);
#endif
	chrome_buffer_object_kunmap(p_priv->p_fence->fence_sync);
	chrome_buffer_object_unref(&p_priv->p_fence->fence_sync);
	kfree(p_priv->p_fence->p_cmd_tmp);
	kfree(p_priv->p_fence);	
	p_priv->p_fence->p_cmd_tmp = NULL;
	p_priv->p_fence = NULL;
}
