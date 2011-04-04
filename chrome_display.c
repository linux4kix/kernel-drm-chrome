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
#include "drm_mode.h"
#include "drm_crtc_helper.h"

#include "via_drv.h"
#include "crtc_hw.h"

static int
via_crtc_cursor_set(struct drm_crtc *crtc, struct drm_file *file_priv,
			uint32_t handle, uint32_t width, uint32_t height)
{
	struct drm_via_private *dev_priv = crtc->dev->dev_private;
	int max_height = 64, max_width = 64, ret = 0;
	struct via_crtc *iga = &dev_priv->iga[0];
	struct drm_device *dev = crtc->dev;
	struct drm_gem_object *obj = NULL;
	bool primary = true;
	uint32_t temp;

	if (crtc != &iga->crtc) {
		iga = &dev_priv->iga[1];
		primary = false;
	}

	if (!handle) {
		/* turn off cursor */
		if (!primary) {
			temp = VIA_READ(VIA_REG_HI_CONTROL1);
			VIA_WRITE(VIA_REG_HI_CONTROL1, temp & 0xFFFFFFFA);
		} else {
			temp = VIA_READ(VIA_REG_HI_CONTROL0);
			VIA_WRITE(VIA_REG_HI_CONTROL0, temp & 0xFFFFFFFA);
		}
		goto unpin;
	}

	if (dev->pdev->device == PCI_DEVICE_ID_VIA_CLE266 ||
	    dev->pdev->device == PCI_DEVICE_ID_VIA_KM400) {
		max_height >>= 1;
		max_width >>= 1;
	}

	if ((height > max_height) || (width > max_width)) {
		DRM_ERROR("bad cursor width or height %d x %d\n", width, height);
		return -EINVAL;
	}

	obj = drm_gem_object_lookup(dev, file_priv, handle);
	if (!obj) {
		DRM_ERROR("Cannot find cursor object %x for crtc %d\n", handle, crtc->base.id);
		return -ENOENT;
	}

	/* set_cursor, show_cursor */
unpin:
	drm_gem_object_unreference_unlocked(iga->cursor_bo);
	iga->cursor_bo = obj;
fail:
	if (ret)
		drm_gem_object_unreference_unlocked(obj);
        return ret;
}

static int
via_crtc_cursor_move(struct drm_crtc *crtc, int x, int y)
{
	return 0;
}

static void
via_crtc_gamma_set(struct drm_crtc *crtc, u16 *red, u16 *green,
			u16 *blue, uint32_t start, uint32_t size)
{
	//int end = (start + size > 256) ? 256 : start + size, i;

	/* userspace palettes are always correct as is *
	for (i = start; i < end; i++) {
		crtc->lut_r[i] = red[i] >> 6;
		crtc->lut_g[i] = green[i] >> 6;
		crtc->lut_b[i] = blue[i] >> 6;
	}
	via_crtc_load_lut(crtc);*/
}

static void
via_crtc_destroy(struct drm_crtc *crtc)
{
	drm_crtc_cleanup(crtc);

	/*bo_unmap(crtc->cursor.nvbo);
	bo_ref(NULL, &crtc->cursor.nvbo);*/
	kfree(crtc);
}

static const struct drm_crtc_funcs via_crtc_funcs = {
	.cursor_set = via_crtc_cursor_set,
	.cursor_move = via_crtc_cursor_move,
	.gamma_set = via_crtc_gamma_set,
	.set_config = drm_crtc_helper_set_config,
	.destroy = via_crtc_destroy,
};

static void
via_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	/* 3D5.36 Bits 5:4 control DPMS */

}

static void
via_crtc_prepare(struct drm_crtc *crtc)
{
	struct drm_via_private *dev_priv = crtc->dev->dev_private;
	struct via_crtc *iga = &dev_priv->iga[0];
	u8 orig;

	if (crtc != &iga->crtc)
		iga = &dev_priv->iga[1];

	/* unlock extended registers */
	seq_outb(iga->vga_regs, 0x10, 0x01);	

	/* unlock CRT registers */
	orig = crtc_inb(iga->vga_regs, 0x47);
	crtc_outb(iga->vga_regs, 0x47, (orig & 0x01));

	regs_init(iga->vga_regs);
}

static void
via_crtc_commit(struct drm_crtc *crtc)
{
}

static bool
via_crtc_mode_fixup(struct drm_crtc *crtc, struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	return true;
}

static int
via_crtc_mode_set(struct drm_crtc *crtc, struct drm_display_mode *mode,
		struct drm_display_mode *adjusted_mode,
		int x, int y, struct drm_framebuffer *old_fb)
{
	struct drm_via_private *dev_priv = crtc->dev->dev_private;
	struct via_crtc *iga = &dev_priv->iga[0];

	if (crtc != &iga->crtc)
		iga = &dev_priv->iga[1];

	crtc_set_regs(mode, iga->vga_regs);
	return 0;
}

static int
via_crtc_mode_set_base(struct drm_crtc *crtc, int x, int y,
			struct drm_framebuffer *old_fb)
{
	return 0;
}

static int
via_crtc_mode_set_base_atomic(struct drm_crtc *crtc, struct drm_framebuffer *fb,
				int x, int y, enum mode_set_atomic state)
{
	return 0;
}

static void 
via_crtc_load_lut(struct drm_crtc *crtc)
{
}

static const struct drm_crtc_helper_funcs via_crtc_helper_funcs = {
	.dpms = via_crtc_dpms,
	.prepare = via_crtc_prepare,
	.commit = via_crtc_commit,
	.mode_fixup = via_crtc_mode_fixup,
	.mode_set = via_crtc_mode_set,
	.mode_set_base = via_crtc_mode_set_base,
	.mode_set_base_atomic = via_crtc_mode_set_base_atomic,
	.load_lut = via_crtc_load_lut,
};

static void 
via_crtc_init(struct drm_device *dev, int index)
{
	struct drm_via_private *dev_priv = dev->dev_private;
	struct drm_crtc *crtc = &dev_priv->iga[index].crtc;

	drm_crtc_init(dev, crtc, &via_crtc_funcs);
	drm_mode_crtc_set_gamma_size(crtc, 256);
	drm_crtc_helper_add(crtc, &via_crtc_helper_funcs);
}

int via_modeset_init(struct drm_device *dev)
{
	int ret = 0, i;

	drm_mode_config_init(dev);

	/* What is the max ? */
	dev->mode_config.min_width = 320;
	dev->mode_config.min_height = 200;
	dev->mode_config.max_width = 2048;
	dev->mode_config.max_height = 2048;

	for (i = 0; i < 2; i++)
		via_crtc_init(dev, i);

	via_analog_init(dev);

	return ret;
}
