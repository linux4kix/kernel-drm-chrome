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
#include "crtc_hw.h"

//regs = devm_ioport_map(&dev->pdev->dev, 0x300, 0x100);

void
regs_init(void __iomem *regs)
{
	int i;

	for (i = 0; i <= 4; i++)
		gra_outb(regs, i, 0x00);	

	gra_outb(regs, 0x05, 0x40);
	gra_outb(regs, 0x06, 0x05); 
	gra_outb(regs, 0x07, 0x0f); 
	gra_outb(regs, 0x08, 0xff); 

	for (i = 0; i <= 0xf; i++)
		att_outb(regs, i, i);

	att_outb(regs, 0x10, 0x41);
	att_outb(regs, 0x11, 0xff);
	att_outb(regs, 0x12, 0x0f);
	att_outb(regs, 0x13, 0x00);
	att_outb(regs, 0x14, 0x00);
}

void
crtc_set_regs(struct drm_display_mode *mode, void __iomem *regs)
{
	/* Calculate our timings */
	int horizDisplay= (mode->crtc_hdisplay >> 3)	- 1;
	int horizStart= (mode->crtc_hsync_start >> 3)	+ 1;
	int horizEnd= (mode->crtc_hsync_end >> 3)	+ 1;
	int horizTotal= (mode->crtc_htotal >> 3)	- 5;
	int horizBlankStart= (mode->crtc_hdisplay >> 3)	- 1;
	int horizBlankEnd= (mode->crtc_htotal >> 3)	- 1;
	int vertDisplay= mode->crtc_vdisplay		- 1;
	int vertStart= mode->crtc_vsync_start		- 1;
	int vertEnd= mode->crtc_vsync_end		- 1;
	int vertTotal= mode->crtc_vtotal		- 2;
	int vertBlankStart= mode->crtc_vdisplay		- 1;
	int vertBlankEnd= mode->crtc_vtotal		- 1;
	int miscReg, i;

	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		vertTotal |= 1;

	/*
	 * compute correct Hsync & Vsync polarity
	 */
	if ((mode->flags & (DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NHSYNC))
	     && (mode->flags & (DRM_MODE_FLAG_PVSYNC | DRM_MODE_FLAG_NVSYNC))) {
		miscReg = 0x23;

	if (mode->flags & DRM_MODE_FLAG_NHSYNC)
		miscReg |= 0x40;
	if (mode->flags & DRM_MODE_FLAG_NVSYNC)
		miscReg |= 0x80;
	} else {
		int vdisplay = mode->vdisplay;

		if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
			vdisplay *= 2;
		if (mode->vscan > 1)
			vdisplay *= mode->vscan;
		if (vdisplay < 400)
			miscReg = 0xA3; /* +hsync -vsync */
		else if (vdisplay < 480)
			miscReg = 0x63; /* -hsync +vsync */
		else if (vdisplay < 768)
			miscReg = 0xE3; /* -hsync -vsync */
		else
			miscReg = 0x23; /* +hsync +vsync */
	}
	miscReg |= (mode->clock_index & 0x03) << 2;
	iowrite8(miscReg, regs + MISC_W);

	/* Sequence registers */
	seq_outb(regs, 0x00, 0x03);	/* 0x03 or 0x00 */
	if (mode->flags & DRM_MODE_FLAG_CLKDIV2)
		seq_outb(regs, 0x01, 0x09);
	else
		seq_outb(regs, 0x01, 0x01);

	seq_outb(regs, 0x02, 0x0f);
	seq_outb(regs, 0x03, 0x00);
	seq_outb(regs, 0x04, 0x0e);

	crtc_outb(regs, 0x00, horizTotal);
	crtc_outb(regs, 0x01, horizDisplay);
	crtc_outb(regs, 0x02, horizBlankStart);
	crtc_outb(regs, 0x03, 0x80 | (horizBlankEnd & 0x1f));
	crtc_outb(regs, 0x04, horizStart);
	crtc_outb(regs, 0x05, ((horizBlankEnd & 0x20) << 2) |
	(horizEnd & 0x1f));
	crtc_outb(regs, 0x06, vertTotal);
	crtc_outb(regs, 0x07,((vertStart & 0x200) >> 2) |
					((vertDisplay & 0x200) >> 3) |
					((vertTotal & 0x200) >> 4) | 0x10 |
					((vertBlankStart & 0x100) >> 5) |
					((vertStart & 0x100) >> 6) |
					((vertDisplay & 0x100) >> 7) |
					((vertTotal & 0x100) >> 8));
	crtc_outb(regs, 0x08, 0x00);
	// Handle double scan
	crtc_outb(regs, 0x09, (0x40 | ((vertBlankStart & 0x200) >> 4)));

	for (i = 0x0a; i < 0x10; i++)
		crtc_outb(regs, i, 0x00);

	crtc_outb(regs, 0x10, vertStart);
	crtc_outb(regs, 0x11, (vertEnd & 0x0f) | 0x20);
	crtc_outb(regs, 0x12, vertDisplay);
	crtc_outb(regs, 0x13, horizDisplay);	// fb->pitch >> 3);
	crtc_outb(regs, 0x14, 0x00);
	crtc_outb(regs, 0x15, vertBlankStart);
	crtc_outb(regs, 0x16, vertBlankEnd + 1);
	crtc_outb(regs, 0x17, 0xc3);
	crtc_outb(regs, 0x18, 0xff);
}
