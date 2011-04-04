/*
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	James Simmons <jsimmons@infradead.org>
 */

#include "drmP.h"
#include "drm_crtc.h"

/* Offset from ATT_IW which is 3C0 */
#define MISC_W          0x02
#define MISC_R          0x0c
#define SEQ_I           0x04
#define SEQ_D           0x05
#define CRT_I           0x14
#define CRT_D           0x15
#define ATT_IW          0x00
#define IS1_R           0x1a
#define GRA_I           0x0e
#define GRA_D           0x0f

static inline void gra_outb(void __iomem *regs, u32 idx, u8 val)
{
	iowrite8(idx, regs + GRA_I);
	wmb();
	iowrite8(val, regs + GRA_D);
	wmb();
}

static inline void seq_outb(void __iomem *regs, u32 idx, u8 val)
{
	iowrite8(idx, regs + SEQ_I);
	wmb();
	iowrite8(idx, regs + SEQ_D);
	wmb();
}

static inline u8 seq_inb(void __iomem *regs, u32 idx)
{
	iowrite8(idx, regs + SEQ_I);
	mb();
	return ioread8(regs + SEQ_D);
}

static inline void crtc_outb(void __iomem *regs, u32 idx, u8 val)
{
	iowrite8(idx, regs + CRT_I);
	wmb();
	iowrite8(val, regs + CRT_D);
	wmb();
}

static inline u8 crtc_inb(void __iomem *regs, u32 idx)
{
	iowrite8(idx, regs + CRT_I);
	mb();
	return ioread8(regs + CRT_D);
}

static inline void att_outb(void __iomem *regs, u32 idx, u8 val)
{
	unsigned char tmp;

	tmp = ioread8(regs + IS1_R);
	iowrite8(idx, regs + ATT_IW);
	iowrite8(val, regs + ATT_IW);
}

static inline void vga_enable_palette(void __iomem *regs)
{
	unsigned char tmp;

	tmp = ioread8(regs + IS1_R);
	mb();
	iowrite8(0x20, regs + ATT_IW);
}

void regs_init(void __iomem *regs);
void crtc_set_regs(struct drm_display_mode *mode, void __iomem *regs);
