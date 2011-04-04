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
#ifndef _CHROME_DMA_H_
#define _CHROME_DMA_H_

/*via command type*/
#define CHROME_CMD_2D 0x10
#define CHROME_CMD_3D 0x20
#define CHROME_CMD_VIDEO 0x40
#define CHROME_CMD_HQV0 0x80
#define CHROME_CMD_HQV1 0x100


struct drm_chrome_dma_manager {
	unsigned int *addr_linear; /* the ring buffer virtual address that we can visit */
	unsigned int dmasize; /* he ring buffer size */
	unsigned int *pstart; /* the ring buffer start virtual address */
	unsigned int *pinusebysw; /* the pointer we have used */
	unsigned int *pfree; /* the free pointer that we can write command */
	unsigned int *pend; /* the ring buffer end virtual address */
	unsigned int maxkickoffsize; /* the max command we can kick off */
	unsigned int physical_start; /* the offset of this ring buffer in gpu space */
	struct mutex command_flush_lock; /* lock for command flush */
	unsigned int cmd_type;
};

enum cmd_request_type {
	CM_REQUEST_BCI,
	CM_REQUEST_DMA,
	CM_REQUEST_RB,
	CM_REQUEST_RB_FORCED_DMA,
	CM_REQUEST_NOTAVAILABLE
};

struct cmd_get_space {
	unsigned int            dwrequestsize; /* the command size we request */
	enum cmd_request_type      hint;
	__volatile__ unsigned long *pcmddata;
};

struct cmd_release_space {
	unsigned int  dwreleasesize;
};

struct drm_chrome_flush {
	int    cmd_size; /* command dword size */
	unsigned int *dma_buf; /* command buffer pointer */
};
extern void chrome_init_dma(struct drm_device *dev);
extern void chrome_dma_fini(struct drm_device *dev);
extern void chrome_init_gart_table(struct drm_device *dev);
extern void chrome_gart_table_fini(struct drm_device *dev);
extern int chrome_ringbuffer_flush(struct drm_device *dev,
			unsigned int *dma_buf, int command_size, bool from_user,
			void *parse_ptr);
extern int chrome_branchbuffer_flush(struct drm_device *dev,
	struct chrome_object *cmd_obj, int command_size, void *parse_ptr);
extern void chrome_kickoff_dma_ring(struct drm_device *dev);
extern int execute_branch_buffer_h6(struct drm_device *dev,
	struct chrome_object *vbo, uint32_t cmd_size);
extern int execute_branch_buffer_h5s2vp1(struct drm_device *dev,
	struct chrome_object *vbo, uint32_t cmd_size);
#endif
