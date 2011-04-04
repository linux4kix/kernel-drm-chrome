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
#include "drm.h"
#include "chrome9_drm.h"
#include "chrome_drv.h"
#include "chrome9_3d_reg.h"
#include "chrome9_dma.h"
#include "chrome9_mm.h"
#include "chrome_object.h"
#include "chrome9_reloc.h"
#include "chrome_fence.h"

#define NULLCOMMANDNUMBER 256
#define PAD_CMD_SIZE 256 /*In DWord count*/
#define MAXALIGNSIZE (0x80*2)
static unsigned int NULL_COMMAND_INV[4] =
	{ 0xCC000000, 0xCD000000, 0xCE000000, 0xCF000000 };


/**
 * set engine to stop state
 */
void
set_agp_ring_buffer_stop(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		(struct drm_via_chrome9_dma_manager *) dev_priv->dma_manager;
	unsigned long agpbuflinearbase, agpbufphysicalbase;
	unsigned int dwreg64, dwreg65, dwpause;
	unsigned int *pfree;

	agpbuflinearbase = (unsigned long) lpcmdmamanager->addr_linear;
	agpbufphysicalbase = (unsigned long) lpcmdmamanager->physical_start;
	pfree = (unsigned int *) lpcmdmamanager->pfree;
	addcmdheader2_invi(pfree, INV_REG_CR_TRANS, INV_PARATYPE_DUMMY);
	do {
		addcmddata_invi(pfree, 0xCCCCCCC0);
		addcmddata_invi(pfree, 0xDDD00000);
	} while ((u32)((unsigned long) pfree) & 0x7f);

	dwpause = (u32) ((unsigned long)pfree - agpbuflinearbase + agpbufphysicalbase - 16);
	dwreg64 = INV_SUBA_HAGPBPL | INV_HWBASL(dwpause);
	dwreg65 = INV_SUBA_HAGPBPID | INV_HWBASH(dwpause) |
				INV_HAGPBPID_STOP;
	setmmioregister(dev_priv->mmio_map,
				INV_REG_CR_TRANS, INV_PARATYPE_PRECR);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg64);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg65);
}


/* ring buffer mechanism setup */
void
set_agp_ring_cmd_inv(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		(struct drm_via_chrome9_dma_manager *) dev_priv->dma_manager;
	unsigned long agpbuflinearbase, agpbufphysicalbase;
	unsigned int *pfree;
	unsigned int dwstart, dwjump, dwend, dwpause, agpcurraddr, agpcurstat;
	unsigned int dwreg60, dwreg61, dwreg62, dwreg63,
			dwreg64, dwreg65, dwreg66, dwreg67, dwreg68;

	/* Set default the read back ID is CR */
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_TRANS,
			INV_PARATYPE_PRECR);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN,
			INV_SUBA_HSETRBGID | INV_HSETRBGID_CR);

	agpbuflinearbase = (unsigned long) lpcmdmamanager->addr_linear;
	agpbufphysicalbase = (unsigned long) lpcmdmamanager->physical_start;
	agpcurstat = getmmioregister(dev_priv->mmio_map, INV_RB_AGPCMD_STATUS);
	
	/* set engine to stop status */
	if (agpcurstat & INV_AGPCMD_INPAUSE) {
		agpcurraddr = getmmioregister(dev_priv->mmio_map,
					INV_RB_AGPCMD_CURRADDR);
		pfree = (unsigned int *) (agpbuflinearbase + agpcurraddr -
						agpbufphysicalbase);

		addcmdheader2_invi(pfree, INV_REG_CR_TRANS,
					INV_PARATYPE_DUMMY);

		do {
			addcmddata_invi(pfree, 0xCCCCCCC0);
			addcmddata_invi(pfree, 0xDDD00000);
		} while ((u32)((unsigned long) pfree) & 0x7f);

		dwpause = (u32) ((unsigned long)pfree - agpbuflinearbase +
						agpbufphysicalbase - 16);

		dwreg64 = INV_SUBA_HAGPBPL | INV_HWBASL(dwpause);
		dwreg65 = INV_SUBA_HAGPBPID | INV_HWBASH(dwpause) |
				INV_HAGPBPID_STOP;

		setmmioregister(dev_priv->mmio_map,
				INV_REG_CR_TRANS, INV_PARATYPE_PRECR);
		setmmioregister(dev_priv->mmio_map,
				INV_REG_CR_BEGIN, dwreg64);
		setmmioregister(dev_priv->mmio_map,
				INV_REG_CR_BEGIN, dwreg65);
#if 0
		while (getmmioregister(dev_priv->mmio_map,
				INV_RB_ENG_STATUS) & INV_ENG_BUSY_ALL);		
#endif
	}

	/* initialize the CR  */
	lpcmdmamanager->pfree = lpcmdmamanager->pstart;
	dwstart = (u32) ((unsigned long)lpcmdmamanager->pstart - agpbuflinearbase +
				agpbufphysicalbase);
	dwend = (u32)((unsigned long)lpcmdmamanager->pend - agpbuflinearbase +
				agpbufphysicalbase);
	addcmdheader2_invi(lpcmdmamanager->pfree,
				INV_REG_CR_TRANS, INV_PARATYPE_DUMMY);
	do {
		addcmddata_invi(lpcmdmamanager->pfree, 0xCCCCCCC0);
		addcmddata_invi(lpcmdmamanager->pfree, 0xDDD00000);
	} while ((u32)((unsigned long) lpcmdmamanager->pfree) & 0x7f);

	dwpause = (u32)((unsigned long)lpcmdmamanager->pfree -
				16 - agpbuflinearbase + agpbufphysicalbase);
	dwjump = 0xFFFFFFF0;

	dwreg60 = INV_SUBA_HAGPBSTL | INV_HWBASL(dwstart) | 0x01;
	dwreg61 = INV_SUBA_HAGPBSTH | INV_HWBASH(dwstart);
	dwreg62 = INV_SUBA_HAGPBENDL | INV_HWBASL(dwend);
	dwreg63 = INV_SUBA_HAGPBENDH | INV_HWBASH(dwend);
	dwreg64 = INV_SUBA_HAGPBPL | INV_HWBASL(dwpause);
	dwreg65 = INV_SUBA_HAGPBPID | INV_HWBASH(dwpause) | INV_HAGPBPID_PAUSE;
	dwreg66 = INV_SUbA_HAGPBJUMPL | INV_HWBASL(dwjump);
	dwreg67 = INV_SUbA_HAGPBJUMPH | INV_HWBASH(dwjump);
	dwreg68 = INV_SUbA_HFTHRCM | INV_HFTHRCM_10 | INV_HAGPBTRIG;
	setmmioregister(dev_priv->mmio_map,
				INV_REG_CR_TRANS, INV_PARATYPE_PRECR);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg60);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg61);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg62);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg63);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg64);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg65);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg66);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg67);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg68);
	lpcmdmamanager->pinusebysw = lpcmdmamanager->pfree;
}

/* reset the hw pause */
static void
rewind_ring_agp_inv(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;
	unsigned int dwpause, dwjump, dwreg64, dwreg65, dwreg66, dwreg67;
	unsigned long agpbuflinearbase =
		(unsigned long) lpcmdmamanager->addr_linear;
	unsigned long agpbufphysicalbase =
		(unsigned long) lpcmdmamanager->physical_start;

	addcmdheader2_invi(lpcmdmamanager->pfree, INV_REG_CR_TRANS,
			   INV_PARATYPE_DUMMY);
	addcmddata_invi(lpcmdmamanager->pfree, 0xCCCCCCC7);

	while ((unsigned long) lpcmdmamanager->pfree & 0x7F)
		addcmddata_invi(lpcmdmamanager->pfree, 0xCCCCCCC7);
	/*410 jump command needs 4*128 align*/
	if (dev->pci_device == 0x7122) {

		dwjump = (u32) ((unsigned long)lpcmdmamanager->pfree
			- agpbuflinearbase + agpbufphysicalbase - 64);
	}else
		dwjump = (u32) ((unsigned long)lpcmdmamanager->pfree
			- agpbuflinearbase + agpbufphysicalbase - 16);

	lpcmdmamanager->pfree = lpcmdmamanager->pstart;
	/* Rewind is also a kick-off, the hardware SPEC defines the min kick-off size
	 of ring buffer. 410 is 128bit, 409 is 5*128bit, so satisfy the Hardware */
	do {
		addcmddata_invi(lpcmdmamanager->pfree, 0xCCCCCCC7);
	} while ((unsigned long) lpcmdmamanager->pfree & 0x7F);

	dwpause = (u32) ((unsigned long)lpcmdmamanager->pfree
		- agpbuflinearbase + agpbufphysicalbase -16);

	dwreg64 = INV_SUBA_HAGPBPL | INV_HWBASL(dwpause);
	dwreg65 = INV_SUBA_HAGPBPID | INV_HWBASH(dwpause) | INV_HAGPBPID_PAUSE;

	dwreg66 = INV_SUbA_HAGPBJUMPL | INV_HWBASL(dwjump);
	dwreg67 = INV_SUbA_HAGPBJUMPH | INV_HWBASH(dwjump);

	setmmioregister(dev_priv->mmio_map, INV_REG_CR_TRANS,
			INV_PARATYPE_PRECR);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg66);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg67);

	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg64);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg65);
	lpcmdmamanager->pinusebysw = lpcmdmamanager->pfree;
}

/* to get the free space for command execute */
static void
get_space_ring_inv(struct drm_device *dev,
		   struct cmd_get_space *lpcmgetspacedata)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;
	unsigned int dwunflushed;
	u32 bufstart, bufend, cursw, curhw, nextsw, boundarycheck;
	unsigned int dwrequestsize = lpcmgetspacedata->dwrequestsize;

	unsigned long agpbuflinearbase = (unsigned long)lpcmdmamanager->addr_linear;
	unsigned long agpbufphysicalbase = 
		(unsigned long)lpcmdmamanager->physical_start;
	dwunflushed =
		(unsigned int) (lpcmdmamanager->pfree - lpcmdmamanager->pstart);

	if (dwrequestsize > lpcmdmamanager->maxkickoffsize) {
		DRM_INFO("too big DMA buffer request!!!\n");
		*lpcmgetspacedata->pcmddata = 0;
		return;
	}

	bufstart =
		(u32)((unsigned long)lpcmdmamanager->pstart - agpbuflinearbase +
		agpbufphysicalbase);
	bufend = (u32)((unsigned long)lpcmdmamanager->pend - agpbuflinearbase +
		agpbufphysicalbase);
	dwrequestsize = lpcmgetspacedata->dwrequestsize << 2;
	nextsw = (u32)((unsigned long)lpcmdmamanager->pfree + dwrequestsize +
		INV_CMDBUF_THRESHOLD * 8 - agpbuflinearbase + agpbufphysicalbase);

	cursw = (u32)((unsigned long)lpcmdmamanager->pfree - agpbuflinearbase +
		agpbufphysicalbase);
	curhw = getmmioregister(dev_priv->mmio_map, INV_RB_AGPCMD_CURRADDR);

	if (nextsw >= bufend) {
		cursw = (u32) ((unsigned long)lpcmdmamanager->pfree -
			agpbuflinearbase + agpbufphysicalbase);
		curhw = getmmioregister(dev_priv->mmio_map,
					INV_RB_AGPCMD_CURRADDR);
		while (curhw > cursw)
			curhw = getmmioregister(dev_priv->mmio_map,
						INV_RB_AGPCMD_CURRADDR);
		/* Sometime the value read from HW is unreliable,
		so need double confirm. */
		curhw = getmmioregister(dev_priv->mmio_map,
					INV_RB_AGPCMD_CURRADDR);
		while (curhw > cursw)
			curhw = getmmioregister(dev_priv->mmio_map,
						INV_RB_AGPCMD_CURRADDR);
		boundarycheck =
			bufstart + dwrequestsize + INV_QW_PAUSE_ALIGN * 16;
		if (boundarycheck >= bufend) {
			DRM_INFO("the request size out of the boundary!!!\n");
		} else {
			/* We need to guarntee the new commands have no chance
			to override the unexected commands or wait until there
			is no unexecuted commands in agp buffer */
			if (cursw <= boundarycheck) {
				curhw = getmmioregister(dev_priv->mmio_map,
					INV_RB_AGPCMD_CURRADDR);
				while (curhw < cursw)
					curhw = getmmioregister(
					dev_priv->mmio_map,
					INV_RB_AGPCMD_CURRADDR);
				/*Sometime the value read from HW is unreliable,
				   so need double confirm. */
				curhw = getmmioregister(dev_priv->mmio_map,
						INV_RB_AGPCMD_CURRADDR);
				while (curhw < cursw) {
					curhw = getmmioregister(
						dev_priv->mmio_map,
						INV_RB_AGPCMD_CURRADDR);
				}
				rewind_ring_agp_inv(dev);
				cursw = (u32)
					((unsigned long)lpcmdmamanager->pfree -
					agpbuflinearbase + agpbufphysicalbase);
				curhw = getmmioregister(dev_priv->mmio_map,
						INV_RB_AGPCMD_CURRADDR);
				/* Waiting until hw pointer jump to start
				and hw pointer will */
				/* equal to sw pointer */
				while (curhw != cursw) {
					curhw = getmmioregister(
						dev_priv->mmio_map,
						INV_RB_AGPCMD_CURRADDR);
				}
			} else {
				curhw = getmmioregister(dev_priv->mmio_map,
						INV_RB_AGPCMD_CURRADDR);

				while (curhw <= boundarycheck) {
					curhw = getmmioregister(
						dev_priv->mmio_map,
						INV_RB_AGPCMD_CURRADDR);
				}
				curhw = getmmioregister(dev_priv->mmio_map,
						INV_RB_AGPCMD_CURRADDR);
				/* Sometime the value read from HW is
				unreliable, so need double confirm. */
				while (curhw <= boundarycheck) {
					curhw = getmmioregister(
						dev_priv->mmio_map,
						INV_RB_AGPCMD_CURRADDR);
				}
				rewind_ring_agp_inv(dev);
			}
		}
	} else {
		/* no need to rewind Ensure unexecuted agp commands will
		not be override by new
		agp commands */
		cursw = (u32) ((unsigned long)lpcmdmamanager->pfree -
			agpbuflinearbase + agpbufphysicalbase);
		curhw = getmmioregister(dev_priv->mmio_map,
					INV_RB_AGPCMD_CURRADDR);

		while ((curhw > cursw) && (curhw <= nextsw))
			curhw = getmmioregister(dev_priv->mmio_map,
					INV_RB_AGPCMD_CURRADDR);

		/* Sometime the value read from HW is unreliable,
		so need double confirm. */
		curhw = getmmioregister(dev_priv->mmio_map,
					INV_RB_AGPCMD_CURRADDR);
		while ((curhw > cursw) && (curhw <= nextsw))
			curhw = getmmioregister(dev_priv->mmio_map,
					INV_RB_AGPCMD_CURRADDR);
	}
	/*return the space handle */
	*lpcmgetspacedata->pcmddata = (unsigned long)lpcmdmamanager->pfree;
}

/* insert some dummy command for address alignment */
static void
release_space_inv(struct drm_device *dev,
		  struct cmd_release_space *lpcmReleaseSpaceData,
		  bool dummy_command)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;
	unsigned int dwreleasesize = lpcmReleaseSpaceData->dwreleasesize;
	int i = 0;

	lpcmdmamanager->pfree += dwreleasesize;

	while (((unsigned long) lpcmdmamanager->pfree) & 0xF) {
		/* not in 4 unsigned ints (16 Bytes) align address,
		insert NULL Commands */
		*lpcmdmamanager->pfree++ = NULL_COMMAND_INV[i & 0x3];
		i++;
	}

	if (dummy_command) {
		addcmdheader2_invi(lpcmdmamanager->pfree, INV_REG_CR_TRANS,
			INV_PARATYPE_DUMMY);
		for (i = 0; i < NULLCOMMANDNUMBER; i++)
			addcmddata_invi(lpcmdmamanager->pfree, 0xCC000000);
	}
}

/*pad 512 byte command, to avoid command cached in WC buffer*/
static void
via_chrome9_pad_command(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		(struct drm_via_chrome9_dma_manager *) dev_priv->dma_manager;
	unsigned int i, *pfree;
	pfree = lpcmdmamanager->pfree;
	/*Just Pad command, not executed by GPU*/
	for (i = 0; i < PAD_CMD_SIZE; i++)
		addcmddata_invi(pfree, 0xCC000000);
	mb();
	return;
}

/* kick of the command */
void
via_chrome9_kickoff_dma_ring(struct drm_device *dev)
{
	unsigned int dwpause, dwreg64, dwreg65;

	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;

	unsigned long agpbuflinearbase =
		(unsigned long)lpcmdmamanager->addr_linear;
	unsigned long agpbufphysicalbase =
		(unsigned long)lpcmdmamanager->physical_start;

	/* 256-bit alignment of AGP pause address */
	if ((unsigned long)lpcmdmamanager->pfree & 0x7f) {
		addcmdheader2_invi(lpcmdmamanager->pfree,
			INV_REG_CR_TRANS, INV_PARATYPE_DUMMY);
		do {
			addcmddata_invi(lpcmdmamanager->pfree, 0xCCCCCCC0);
			addcmddata_invi(lpcmdmamanager->pfree, 0xDDD00000);
		} while ((unsigned long)lpcmdmamanager->pfree & 0x7f);
	}

	via_chrome9_pad_command(dev);

	dwpause = (unsigned int)((unsigned long)lpcmdmamanager->pfree
		- agpbuflinearbase + agpbufphysicalbase - 16);

	dwreg64 = INV_SUBA_HAGPBPL | INV_HWBASL(dwpause);
	dwreg65 = INV_SUBA_HAGPBPID | INV_HWBASH(dwpause) | INV_HAGPBPID_PAUSE;

	setmmioregister(dev_priv->mmio_map, INV_REG_CR_TRANS,
			INV_PARATYPE_PRECR);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg64);
	setmmioregister(dev_priv->mmio_map, INV_REG_CR_BEGIN, dwreg65);
	lpcmdmamanager->pinusebysw = lpcmdmamanager->pfree;

}

void via_chrome9_init_gart_table(struct drm_device *dev)
{
	struct drm_via_chrome9_private *p_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	unsigned int gartoffset = p_priv->pcie_gart_start;
	u8 sr6a, sr6b, sr6c, sr6f;

	/* enable gti write */
	VIA_CHROME9_WRITE8(0x83c4, 0x6c);
	sr6c = VIA_CHROME9_READ8(0x83c5);
	sr6c &= 0x7F;
	VIA_CHROME9_WRITE8(0x83c5, sr6c);

	/* set the base address of gart table */
	sr6a = (gartoffset & 0xff000) >> 12;
	VIA_CHROME9_WRITE8(0x83c4, 0x6a);
	VIA_CHROME9_WRITE8(0x83c5, sr6a);

	sr6b = (gartoffset & 0xff000) >> 20;
	VIA_CHROME9_WRITE8(0x83c4, 0x6b);
	VIA_CHROME9_WRITE8(0x83c5, sr6b);

	VIA_CHROME9_WRITE8(0x83c4, 0x6c);
	sr6c = VIA_CHROME9_READ8(0x83c5);
	sr6c |= ((gartoffset >> 28) & 0x01);
	VIA_CHROME9_WRITE8(0x83c5, sr6c);

	/* flush the gti cache */
	VIA_CHROME9_WRITE8(0x83c4, 0x6f);
	sr6f = VIA_CHROME9_READ8(0x83c5);
	sr6f |= 0x80;
	VIA_CHROME9_WRITE8(0x83c5, sr6f);

	/* disable the gti write */
	VIA_CHROME9_WRITE8(0x83c4, 0x6c);
	sr6c = VIA_CHROME9_READ8(0x83c5);
	sr6c |= 0x80;
	VIA_CHROME9_WRITE8(0x83c5, sr6c);

}

void via_chrome9_gart_table_fini(struct drm_device *dev)
{
	struct drm_via_chrome9_private *p_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	u8 sr6c;

	/* enable gti write */
	VIA_CHROME9_WRITE8(0x83c4, 0x6c);
	sr6c = VIA_CHROME9_READ8(0x83c5);
	sr6c &= 0x7F;
	VIA_CHROME9_WRITE8(0x83c5, sr6c);

	/* release the gart table */
	via_chrome9_buffer_object_kunmap(p_priv->agp_gart);
	via_chrome9_buffer_object_unref(&p_priv->agp_gart);
	/*release gart_table shadow*/
	via_chrome9_buffer_object_kunmap(p_priv->pm_backup.agp_gart_shadow);
	via_chrome9_buffer_object_unref(&p_priv->pm_backup.agp_gart_shadow);
	p_priv->pcie_gart_map = NULL;
}
/**
 *  Execute commands in branch buffer
 *  vbo : branch buffer BO
 *  cmd_size : cmd size in branch buffer (in Dword count)
*/

int
execute_branch_buffer_h5s2vp1(struct drm_device *dev,
	struct via_chrome9_object *vbo, uint32_t cmd_size)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	struct cmd_get_space getspace;
	struct cmd_release_space releasespace;
	uint32_t *pcmddata = NULL, *pcmdbuf = NULL;
	uint32_t buf_offset = vbo->bo.offset;
	uint32_t location,i;

	/*Set branch buffer BO location to PCIE*/
	location = 0x1;
	/*alloc space in ringbuffer*/
	getspace.dwrequestsize = AGP_REQ_FIFO_DEPTH*4+28;
	getspace.dwrequestsize += NULLCOMMANDNUMBER + 4 +
							MAXALIGNSIZE + PAD_CMD_SIZE;
	getspace.pcmddata = (unsigned long *) &pcmddata;
	get_space_ring_inv(dev, &getspace);

	if (pcmddata) {
		pcmdbuf = pcmddata;
		ADDCmdHeader4X_INVI(pcmdbuf, (unsigned int)(buf_offset),
			cmd_size, INV_AGPHEADER40, NEW_BRANCH_HIERARCHY_1ST, 0, location);
#if 0
		addcmdheader82_invi(pcmdbuf, INV_REG_CR_TRANS,
			INV_ParaType_Dummy);
#endif
		for (i = 0 ; i < AGP_REQ_FIFO_DEPTH/2; i++) {
			addcmdheader2_invi(pcmdbuf, INV_REG_CR_TRANS,
			INV_ParaType_Dummy);
			addcmddata_invi(pcmdbuf, 0xCC0CCCCC);
		}
		releasespace.dwreleasesize = pcmdbuf - pcmddata;

		release_space_inv(dev, &releasespace, 1);
		dev_priv->engine_ops.cmdbuffer_ops.kickoff_dma_ring(dev);
	} else {
		DRM_INFO("No enough DMA space");
		return -ENOMEM;
	}
	return 0;
}

int
execute_branch_buffer_h6(struct drm_device *dev,
	struct via_chrome9_object *vbo, uint32_t cmd_size)
{	
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	struct cmd_get_space getspace;
	struct cmd_release_space releasespace;
	uint32_t i, dwReg68, dwReg69, dwReg6A, dwReg6B;
	uint32_t *pcmddata = NULL, *pcmdbuf = NULL;
	uint32_t buf_offset = vbo->bo.offset;
	uint32_t location;

	/*Set branch buffer BO location to PCIE*/
	location = 0x1;
	/*alloc space in ringbuffer*/
	getspace.dwrequestsize = AGP_REQ_FIFO_DEPTH*4+28;
	getspace.dwrequestsize += NULLCOMMANDNUMBER + 4 +
							MAXALIGNSIZE + PAD_CMD_SIZE;
	getspace.pcmddata = (unsigned long *) &pcmddata;
	get_space_ring_inv(dev, &getspace);

	if (pcmddata) {
		pcmdbuf = pcmddata;
		dwReg68 = 0x68000000;
		dwReg69 = 0x69000000 | ((buf_offset & 0x00fffffe));
		dwReg6A = 0x6A000000 | ((buf_offset & 0xff000000) >> 24) |
			location << 22; 
		dwReg6B = 0x6B000000 | (cmd_size >> 2);
		addcmdheader2_invi(pcmdbuf, INV_REG_CR_TRANS,
			INV_ParaType_CR);
		addcmddata_invi(pcmdbuf, dwReg68);
		addcmddata_invi(pcmdbuf, dwReg69);
		addcmddata_invi(pcmdbuf, dwReg6A);
		addcmddata_invi(pcmdbuf, dwReg6B);

		addcmdheader82_invi(pcmdbuf, INV_REG_CR_TRANS,
			INV_ParaType_Dummy);

		for (i = 0 ; i < AGP_REQ_FIFO_DEPTH/2; i++) {
			addcmdheader2_invi(pcmdbuf, INV_REG_CR_TRANS,
			INV_ParaType_Dummy);
			addcmddata_invi(pcmdbuf, 0xCC0CCCCC);
		}
		releasespace.dwreleasesize = pcmdbuf - pcmddata;

		release_space_inv(dev, &releasespace, 1);
		dev_priv->engine_ops.cmdbuffer_ops.kickoff_dma_ring(dev);
	} else {
		DRM_INFO("No enough DMA space");
		return -ENOMEM;
	}
	return 0;
}

void via_chrome9_init_dma(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager = NULL;

	dev_priv->dma_manager =
		kmalloc(sizeof(struct drm_via_chrome9_dma_manager), GFP_KERNEL);
	if (!dev_priv->dma_manager) {
		DRM_ERROR("could not allocate system for dma_manager!\n");
		return;
	}
	lpcmdmamanager =
		(struct drm_via_chrome9_dma_manager *) dev_priv->dma_manager;
	mutex_init(&lpcmdmamanager->command_flush_lock);	
	lpcmdmamanager->dmasize = dev_priv->ring_buffer_size;
	lpcmdmamanager->dmasize /= sizeof(unsigned int);
	lpcmdmamanager->pstart = dev_priv->pcie_ringbuffer_map;
	lpcmdmamanager->pfree = lpcmdmamanager->pstart;
	lpcmdmamanager->pinusebysw = lpcmdmamanager->pstart;
	lpcmdmamanager->addr_linear = dev_priv->pcie_ringbuffer_map;
	lpcmdmamanager->maxkickoffsize = lpcmdmamanager->dmasize;
	lpcmdmamanager->physical_start = dev_priv->agp_ringbuffer->bo.offset;
	lpcmdmamanager->pend = lpcmdmamanager->addr_linear +
					lpcmdmamanager->dmasize;
	lpcmdmamanager->cmd_type = 0;

	set_agp_ring_cmd_inv(dev);
}

void via_chrome9_dma_fini(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;

	/* stop the cr engine */
	set_agp_ring_buffer_stop(dev);

	/* release the ring buffer */
	via_chrome9_buffer_object_kunmap(dev_priv->agp_ringbuffer);
	via_chrome9_buffer_object_unref(&dev_priv->agp_ringbuffer);
	dev_priv->agp_ringbuffer = NULL;

	/* release the dma manager */
	kfree(dev_priv->dma_manager);
	dev_priv->dma_manager = NULL;
}

int via_chrome9_ringbuffer_flush(struct drm_device *dev, unsigned int *dma_buf,
					int command_size, bool from_user,
					void *parse_ptr)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_gem_flush_parse *parse =
		(struct drm_via_chrome9_gem_flush_parse *)parse_ptr;
	struct cmd_get_space getspace;
	struct cmd_release_space releasespace;
	struct via_chrome9_flush_buffer *reloc_buffer;
	struct list_head *p;
	unsigned long *pcmddata = NULL;
	int ret = 0;
	bool dummy_command = true;

	if (command_size <= 0) {
		printk(KERN_ALERT "command size fault \n");
		return -EINVAL;
	}

	getspace.dwrequestsize = command_size;
	getspace.dwrequestsize += (NULLCOMMANDNUMBER + 4 + MAXALIGNSIZE + PAD_CMD_SIZE);
	getspace.pcmddata = (unsigned long *) &pcmddata;
	get_space_ring_inv(dev, &getspace);
	if (pcmddata) {
		if (from_user) {
#if VIA_CHROME9_VERIFY_ENABLE
			if ((ret = via_chrome9_verify_user_command(
					(uint32_t *)dma_buf, command_size << 2, dev, 0))) {
				return -EINVAL;
			}
#endif

#if VIA_CHROME9_VERIFY_ENABLE
			memcpy((void *)pcmddata, (void *)dev_priv->verify_buff,
					command_size << 2);
#else
			ret = copy_from_user((int *)pcmddata, dma_buf,
					command_size << 2);
#endif
			if (ret) {
				DRM_ERROR("In function via_chrome9_ringbuffer_flush,"
					"copy_from_user is fault. \n");
				return -EINVAL;
			}
		} else {
			memcpy((void *)pcmddata,
					(void *)dma_buf, command_size << 2);
		}

	} else {
		DRM_INFO("No enough ring buffer space");
		ret = -ENOMEM;
	}
#if VIA_CHROME9_VERIFY_ENABLE
	if (from_user)
		via_chrome9_verify_user_command_done(dev);
#endif
	if (from_user) {
		if (parse->need_correct) {
			list_for_each(p, &parse->valid_list) {
				reloc_buffer = list_entry(
					p, struct via_chrome9_flush_buffer, list);
				via_chrome9_reloc_valid(parse, pcmddata,
					reloc_buffer->reloc_count);
			}
		}
		if (parse->fence != NULL)
			dummy_command = false;
	}

	releasespace.dwreleasesize = command_size;
	release_space_inv(dev, &releasespace, dummy_command);
	dev_priv->engine_ops.cmdbuffer_ops.kickoff_dma_ring(dev);


	return ret;
}

int via_chrome9_branchbuffer_flush(struct drm_device *dev,
	struct via_chrome9_object *cmd_obj, int command_size, void *parse_ptr)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_gem_flush_parse *parse =
		(struct drm_via_chrome9_gem_flush_parse *)parse_ptr;
	struct via_chrome9_cmdbuffer_ops *cmdbuffer_ops =
		&dev_priv->engine_ops.cmdbuffer_ops;
	struct via_chrome9_flush_buffer *reloc_buffer;
	struct list_head *p;
	int ret = 0;

	set_bit(VIA_CHROME9_BO_FLAG_CMD_FLUSHING, &cmd_obj->flags);
	/*Unmap BO from userspace*/
	ttm_bo_unmap_virtual(&cmd_obj->bo);

	/*map cmd buffer BO*/
	via_chrome9_buffer_object_kmap(cmd_obj, NULL);

	/*verify command*/
#if VIA_CHROME9_VERIFY_ENABLE
	ret = via_chrome9_verify_command_stream(cmd_obj->kmap.virtual,
		command_size << 2, dev, 0);
	if (ret) {
		DRM_ERROR("The user command has security issue.\n");
		return -EINVAL;
	}
#endif
	/* may needs correct */
	if (parse->need_correct) {
		list_for_each(p, &parse->valid_list) {
			reloc_buffer = list_entry(
				p, struct via_chrome9_flush_buffer, list);
			via_chrome9_reloc_valid(parse, cmd_obj->kmap.virtual,
				reloc_buffer->reloc_count);
		}
	}

	/*Pin branch Buffer to the TT memory(WC)*/
	ret = via_chrome9_bo_pin(cmd_obj, VIA_CHROME9_GEM_DOMAIN_GTT, NULL);
	if (ret) {
		DRM_ERROR("Branch buffer Pin failed!\n ");
		via_chrome9_fence_unref((struct via_chrome9_fence_object **)&cmd_obj->bo.sync_obj);
		return ret;
	}

	/*add fence to cmd buffer BO*/
	if (likely(parse->fence))
		cmd_obj->bo.sync_obj = via_chrome9_fence_ref(parse->fence);

	/*execuate branch buffer command*/
	cmdbuffer_ops->execute_branch_buffer(dev, cmd_obj, command_size);

	/*unmap BO*/
	via_chrome9_buffer_object_kunmap(cmd_obj);

	return ret;
}

