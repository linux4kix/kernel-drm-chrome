/*
 * Copyright 2011 James Simmons <jsimmons@infradead.org>. All Rights Reserved.
 *
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
 */

#include "drmP.h"
#include "chrome_drm.h"
#include "chrome_drv.h"

#include "drm_pciids.h"

static int
cle266_mem_type(struct drm_chrome_private *dev_priv, struct pci_dev *bridge)
{
	u8 type, fsb, freq;
	int ret;

	ret = pci_read_config_byte(bridge, 0x54, &fsb);
	if (ret)
		return ret;
	ret = pci_read_config_byte(bridge, 0x69, &freq);
	if (ret)
		return ret;

	freq >>= 6;
	fsb >>= 6;

	/* FSB frequency */
	switch (fsb) {
	case 0x01: /* 100MHz */
		switch (freq) {
		case 0x00:
			freq = 100;
			break;
		case 0x01:
			freq = 133;
			break;
		case 0x02:
			freq = 66;
			break;
		default:
			freq = 0;
			break;
		}
		break;

	case 0x02: /* 133 MHz */
	case 0x03:
		switch (freq) {
		case 0x00:
			freq = 133;
			break;
		case 0x02:
			freq = 100;
			break;
		default:
			freq = 0;
			break;
		}
		break;
	default:
		freq = 0;
		break;
	}

	ret = pci_read_config_byte(bridge, 0x60, &fsb);
	if (ret)
		return ret;
	ret = pci_read_config_byte(bridge, 0xE3, &type);
	if (ret)
		return ret;

	/* On bank 2/3 */
	if (type & 0x02)
		fsb >>= 2;

	/* Memory type */
	switch (fsb & 0x03) {
	case 0x00: /* SDR */
		switch (freq) {
		case 66:
			dev_priv->vram_type = VIA_MEM_SDR66;
			break;
		case 100:
			dev_priv->vram_type = VIA_MEM_SDR100;
			break;
		case 133:
			dev_priv->vram_type = VIA_MEM_SDR133;
		default:
			break;
		}
		break;

	case 0x02: /* DDR */
		switch (freq) {
		case 100:
			dev_priv->vram_type = VIA_MEM_DDR_200;
			break;
		case 133:
			dev_priv->vram_type = VIA_MEM_DDR_266;
		default:
			break;
		}
	default:
		break;
	}
	return ret;
}

static int
km400_mem_type(struct drm_chrome_private *dev_priv, struct pci_dev *bridge)
{
	u8 fsb, freq, rev;
	int ret;

	ret = pci_read_config_byte(bridge, 0xF6, &rev);
	if (ret)
		return ret;
	ret = pci_read_config_byte(bridge, 0x54, &fsb);
	if (ret)
		return ret;
	ret = pci_read_config_byte(bridge, 0x69, &freq);
	if (ret)
		return ret;

	freq >>= 6;
	fsb >>= 6;

	/* KM400 */
	if (rev < 0x80) {
		/* FSB frequency */
		switch (fsb) {
		case 0x00:
			switch (freq) {
			case 0x00:
				dev_priv->vram_type = VIA_MEM_DDR_200;
				break;
			case 0x01:
				dev_priv->vram_type = VIA_MEM_DDR_266;
				break;
			case 0x02:
				dev_priv->vram_type = VIA_MEM_DDR_400;
				break;
			case 0x03:
				dev_priv->vram_type = VIA_MEM_DDR_333;
			default:
				break;
			}
			break;

		case 0x01:
			switch (freq) {
			case 0x00:
				dev_priv->vram_type = VIA_MEM_DDR_266;
				break;
			case 0x01:
				dev_priv->vram_type = VIA_MEM_DDR_333;
				break;
			case 0x02:
				dev_priv->vram_type = VIA_MEM_DDR_400;
			default:
				break;
			}
			break;

		case 0x02:
		case 0x03:
			switch (freq) {
			case 0x00:
				dev_priv->vram_type = VIA_MEM_DDR_333;
				break;
			case 0x02:
				dev_priv->vram_type = VIA_MEM_DDR_400;
				break;
			case 0x03:
				dev_priv->vram_type = VIA_MEM_DDR_266;	
			default:
				break;
			}
		default:
			break;
		}
	} else {
		/* KM400A */
		pci_read_config_byte(bridge, 0x67, &rev);
		if (rev & 0x80)
			freq |= 0x04;

		switch (fsb) {
		case 0x00:
			switch (freq) {
			case 0x00:
				dev_priv->vram_type = VIA_MEM_DDR_200;
				break;
			case 0x01:
				dev_priv->vram_type = VIA_MEM_DDR_266;
				break;
			case 0x03:
				dev_priv->vram_type = VIA_MEM_DDR_333;
				break;
			case 0x07:
				dev_priv->vram_type = VIA_MEM_DDR_400;
				break;
			default:
				dev_priv->vram_type = VIA_MEM_NONE;
				break;
			}
			break;

		case 0x01:
			switch (freq) {
			case 0x00:
				dev_priv->vram_type = VIA_MEM_DDR_266;
				break;
			case 0x01:
				dev_priv->vram_type = VIA_MEM_DDR_333;
				break;
			case 0x03:
				dev_priv->vram_type = VIA_MEM_DDR_400;
			default:
				break;
			}
			break;

		case 0x02:
			switch (freq) {
			case 0x00:
				dev_priv->vram_type = VIA_MEM_DDR_400;
				break;
			case 0x04:
				dev_priv->vram_type = VIA_MEM_DDR_333;
				break;
			case 0x06:
				dev_priv->vram_type = VIA_MEM_DDR_266;
			default:
				break;
			}
			break;

		case 0x03:
			switch (freq) {
			case 0x00:
				dev_priv->vram_type = VIA_MEM_DDR_333;
				break;
			case 0x01:
				dev_priv->vram_type = VIA_MEM_DDR_400;
				break;
			case 0x04:
				dev_priv->vram_type = VIA_MEM_DDR_266;
			default:
				break;
			}
		default:
			break;
		}
	}
	return ret;
}

static int
p4m800_mem_type(struct drm_chrome_private *dev_priv, struct pci_bus *bus,
		struct pci_dev *fn3)
{
	struct pci_dev *fn4 = pci_get_slot(bus, PCI_DEVFN(0, 4));
	int ret, freq = 0;
	u8 type, fsb;

	/* VIA Scratch region */
	ret = pci_read_config_byte(fn4, 0xF3, &fsb);
	if (ret) {
		pci_dev_put(fn4);
		return ret;
	}

	switch (fsb >> 5) {
	case 0:
		freq = 3; /* 100 MHz */
		break;	
	case 1:
		freq = 4; /* 133 MHz */
		break;
	case 3:
		freq = 5; /* 166 MHz */
		break;
	case 2:
		freq = 6; /* 200 MHz */
		break;
	case 4:
		freq = 7; /* 233 MHz */
	default:
		break;
	}
	pci_dev_put(fn4);

	ret = pci_read_config_byte(fn3, 0x68, &type);
	if (ret)
		return ret;
	type &= 0x0f;

	if (type & 0x02)
		freq -= type >> 2;
	else {
		freq += type >> 2;
		if (type & 0x01)
			freq++;
	}

	switch (freq) {
	case 0x03:
		dev_priv->vram_type = VIA_MEM_DDR_200;
		break;
	case 0x04:
		dev_priv->vram_type = VIA_MEM_DDR_266;	
		break;
	case 0x05:
		dev_priv->vram_type = VIA_MEM_DDR_333;
		break;
	case 0x06:
		dev_priv->vram_type = VIA_MEM_DDR_400;
	default:
		break;
	}
	return ret;	
}

static int
km8xx_mem_type(struct drm_chrome_private *dev_priv)
{
	struct pci_dev *dram, *misc;
	int ret = -ENXIO;
	u8 type, tmp;

	dram = pci_get_device(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_K8_NB_MEMCTL, NULL);
	if (dram) {
		misc = pci_get_device(PCI_VENDOR_ID_AMD,
					PCI_DEVICE_ID_AMD_K8_NB_MISC, NULL);

		ret = pci_read_config_byte(misc, 0xFD, &type);
		if (type) {
			pci_read_config_byte(dram, 0x94, &type);
			switch (type & 0x03) {
			case 0x00:
				dev_priv->vram_type = VIA_MEM_DDR2_400;
				break;
			case 0x01:
				dev_priv->vram_type = VIA_MEM_DDR2_533;
				break;
			case 0x02:
				dev_priv->vram_type = VIA_MEM_DDR2_667;
				break;
			case 0x03:
				dev_priv->vram_type = VIA_MEM_DDR2_800;
			default:
				break;
			}
		} else {
			pci_read_config_byte(dram, 0x96, &type);
			type >>= 4;
			type &= 0x07;

			switch (type) {
			case 0x00:
				dev_priv->vram_type = VIA_MEM_DDR_200;
				break;
			case 0x02:
				dev_priv->vram_type = VIA_MEM_DDR_266;
				break;
			case 0x05:
				dev_priv->vram_type = VIA_MEM_DDR_333;
				break;
			case 0x07:
				dev_priv->vram_type = VIA_MEM_DDR_400;
			default:
				break;
			}
		}
	}

	/* AMD 10h DRAM Controller */
	dram = pci_get_device(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_10H_NB_DRAM, NULL);
	if (dram) {
		ret = pci_read_config_byte(misc, 0x94, &tmp);
		if (ret)
			return ret;
		ret = pci_read_config_byte(misc, 0x95, &type);
		if (ret)
			return ret;

        	if (type & 0x01) {	/* DDR3 */
			switch(tmp & 0x07) {
			case 0x03:
				dev_priv->vram_type = VIA_MEM_DDR3_800;
				break;
			case 0x04:
				dev_priv->vram_type = VIA_MEM_DDR3_1066;
				break;
			case 0x05:
				dev_priv->vram_type = VIA_MEM_DDR3_1333;
				break;
			case 0x06:
				dev_priv->vram_type = VIA_MEM_DDR3_1600;
			default:
				break;
			}
        	} else {		/* DDR2 */
			switch(tmp & 0x07) {
			case 0x00:
				dev_priv->vram_type = VIA_MEM_DDR2_400;
				break;
			case 0x01:
				dev_priv->vram_type = VIA_MEM_DDR2_533;
				break;
			case 0x02:
				dev_priv->vram_type = VIA_MEM_DDR2_667;
				break;
			case 0x03:
				dev_priv->vram_type = VIA_MEM_DDR2_800;
				break;
			case 0x04:
				dev_priv->vram_type = VIA_MEM_DDR2_1066;
			default:
				break;
			}
		}
	}

	/* AMD 11h DRAM Controller */
	dram = pci_get_device(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_11H_NB_DRAM, NULL);
	if (dram) {
		ret = pci_read_config_byte(misc, 0x94, &type);
		if (ret)
			return ret;

		switch(tmp & 0x07) {
		case 0x01:
			dev_priv->vram_type = VIA_MEM_DDR2_533;
			break;
		case 0x02:
			dev_priv->vram_type = VIA_MEM_DDR2_667;
			break;
		case 0x03:
			dev_priv->vram_type = VIA_MEM_DDR2_800;
		default:
			break;
		}
	}
	return ret;
}

static int
cn400_mem_type(struct drm_chrome_private *dev_priv, struct pci_bus *bus,
		struct pci_dev *fn3)
{
	struct pci_dev *fn2 = pci_get_slot(bus, PCI_DEVFN(0, 2));
	int ret, freq = 0;
	u8 type, fsb;

	ret = pci_read_config_byte(fn2, 0x54, &fsb);
	if (ret) {
		pci_dev_put(fn2);
		return ret;
	}

	switch (fsb >> 5) {
	case 0:
		freq = 3; /* 100 MHz */
		break;	
	case 1:
		freq = 4; /* 133 MHz */
		break;
	case 3:
		freq = 5; /* 166 MHz */
		break;
	case 2:
		freq = 6; /* 200 MHz */
		break;
	case 4:
		freq = 7; /* 233 MHz */
	default:
		break;
	}
	pci_dev_put(fn2);

	ret = pci_read_config_byte(fn3, 0x68, &type);
	if (ret)
		return ret;
	type &= 0x0f;

	if (type & 0x01)
		freq += 1 + (type >> 2);
	else
		freq -= 1 + (type >> 2);

	switch (freq) {
	case 0x03:
		dev_priv->vram_type = VIA_MEM_DDR_200;
		break;
	case 0x04:
		dev_priv->vram_type = VIA_MEM_DDR_266;	
		break;
	case 0x05:
		dev_priv->vram_type = VIA_MEM_DDR_333;
		break;
	case 0x06:
		dev_priv->vram_type = VIA_MEM_DDR_400;
	default:
		break;
	}
	return ret;
}

static int
cn700_mem_type(struct drm_chrome_private *dev_priv, struct pci_dev *fn3)
{
	int ret;
	u8 tmp;
	
	ret = pci_read_config_byte(fn3, 0x90, &tmp);
	if (!ret) {
		switch(tmp & 0x07) {
		case 0x00:
			dev_priv->vram_type = VIA_MEM_DDR_200;
			break;
		case 0x01:
			dev_priv->vram_type = VIA_MEM_DDR_266;
			break;
		case 0x02:
			dev_priv->vram_type = VIA_MEM_DDR_333;
			break;
		case 0x03:
			dev_priv->vram_type = VIA_MEM_DDR_400;
			break;
		case 0x04:
			dev_priv->vram_type = VIA_MEM_DDR2_400;
			break;
		case 0x05:
			dev_priv->vram_type = VIA_MEM_DDR2_533;
		default:
			break;
		}
	}
	return ret;
}

static int
cx700_mem_type(struct drm_chrome_private *dev_priv, struct pci_dev *fn3)
{
	u8 type, clock;
	int ret;

	ret = pci_read_config_byte(fn3, 0x90, &clock);
	if (ret)
		return ret;
	ret = pci_read_config_byte(fn3, 0x6C, &type);
	if (ret)
		return ret;
	type &= 0x40;
	type >>= 6;

	switch (type) {
	case 0:
		switch (clock) {
		case 0:
			dev_priv->vram_type = VIA_MEM_DDR_200;
			break;
		case 1:
			dev_priv->vram_type = VIA_MEM_DDR_266;
			break;
		case 2:
			dev_priv->vram_type = VIA_MEM_DDR_333;
			break;
		case 3:
			dev_priv->vram_type = VIA_MEM_DDR_400;
		default:
			break;
		}
		break;

	case 1:
		switch (clock) {
		case 3:
			dev_priv->vram_type = VIA_MEM_DDR2_400;
			break;
		case 4:
			dev_priv->vram_type = VIA_MEM_DDR2_533;
		default:
			break;
		}
	default:
		break;
	}
	return ret;
}

int chrome_detect_vram(struct drm_device *dev)
{
	struct drm_chrome_private *dev_priv = dev->dev_private;
	struct pci_dev *bridge = NULL, *fn3 = NULL;
	u8 type = VIA_MEM_NONE, size;
	int vram_size = 0, ret = 0;
	char *name = "Unknown";
	struct pci_bus *bus;

	bus = pci_find_bus(0, 0);
	if (bus == NULL) {
		ret = -EINVAL;
		goto out_err;
	}

	bridge = pci_get_slot(bus, PCI_DEVFN(0, 0));
	fn3 = pci_get_slot(bus, PCI_DEVFN(0, 3));

	if (!bridge || !fn3) {
		ret = -EINVAL;
		goto out_err;
	}

	switch (bridge->device) {

	/* CLE266 */
	case PCI_DEVICE_ID_VIA_862X_0:
		ret = cle266_mem_type(dev_priv, bridge);
		if (ret)
			goto out_err;

		ret = pci_read_config_byte(bridge, 0xE1, &size);
		if (ret)
			goto out_err;
		vram_size = (1 << ((size & 0x70) >> 4)) << 20;
		break;

	/* KM400/KN400 */
	case PCI_DEVICE_ID_VIA_8378_0:
		ret = km400_mem_type(dev_priv, bridge);

		ret = pci_read_config_byte(bridge, 0xE1, &size);
		if (ret)
			goto out_err;
		vram_size = (1 << ((size & 0x70) >> 4)) << 20;
		break;

	/* P4M800 */
	case PCI_DEVICE_ID_VIA_3296_0:
		type = p4m800_mem_type(dev_priv, bus, fn3);		

		ret = pci_read_config_byte(fn3, 0xA1, &size);
		if (ret)
			goto out_err;
		vram_size = (1 << ((size & 0x70) >> 4)) << 20;
		break;

	/* K8M800/K8N800 */
	case PCI_DEVICE_ID_VIA_8380_0: 
	/* K8M890 */
	case PCI_DEVICE_ID_VIA_VT3336:
		ret = pci_read_config_byte(fn3, 0xA1, &size);
		if (ret)
			goto out_err;
		vram_size = (1 << ((size & 0x70) >> 4)) << 20;

		if (bridge->device == PCI_DEVICE_ID_VIA_VT3336)
			vram_size <<= 2;
		
		ret = km8xx_mem_type(dev_priv);
		if (ret)
			goto out_err;
		break;

	/* CN400/PM800/PM880 */
	case PCI_DEVICE_ID_VIA_PX8X0_0:
		ret = pci_read_config_byte(fn3, 0xA1, &size);
		if (ret)
			goto out_err;
		vram_size = (1 << ((size & 0x70) >> 4)) << 20;

		ret = cn400_mem_type(dev_priv, bus, fn3);
		if (ret)
			goto out_err;
		break;

	/* CN700/VN800/P4M800CE/P4M800Pro */
	case PCI_DEVICE_ID_VIA_P4M800CE:
	/* P4M900/VN896/CN896 */
	case PCI_DEVICE_ID_VIA_VT3364:
	/* VX800 */
	case PCI_DEVICE_ID_VIA_VT3353:
	/* VX855 */
	case PCI_DEVICE_ID_VIA_VT3409:
	/* VX900 */
	case PCI_DEVICE_ID_VIA_VT3410:
		ret = pci_read_config_byte(fn3, 0xA1, &size);
		if (ret)
			goto out_err;
		vram_size = (1 << ((size & 0x70) >> 4)) << 20;

		if (bridge->device != PCI_DEVICE_ID_VIA_P4M800CE)
			vram_size <<= 2;

		ret = cn700_mem_type(dev_priv, fn3);
		if  (ret)
			goto out_err;
		break;

	/* CX700/VX700 */
	case PCI_DEVICE_ID_VIA_VT3324:
	/* P4M890 */
	case PCI_DEVICE_ID_VIA_P4M890:
		ret = pci_read_config_byte(fn3, 0xA1, &size);
		if (ret)
			goto out_err;
		vram_size = (1 << ((size & 0x70) >> 4)) << 22;

		ret = cx700_mem_type(dev_priv, fn3);
		if (ret)
			goto out_err;
		break;

	default:
		DRM_ERROR("Unknown North Bridge device 0x%04x.\n", bridge->device);
		goto out_err;
	}

	/*
	 * Detect VRAM start.
	 */
	if (fn3 != NULL && (fn3->device == 0x3204)) {
		pci_read_config_byte(fn3, 0x47, &size);
		dev_priv->vram_start = size << 24;
		dev_priv->vram_start -= vram_size;
	} else {
		int index = (fn3->device == 0x7122 ? 1 : 0);

		dev_priv->vram_start = pci_resource_start(dev->pdev, index);
	}

	switch (dev_priv->vram_type) {
	case VIA_MEM_SDR66:
		name = "SDR 66";
		break;
	case VIA_MEM_SDR100:
		name = "SDR 100";
		break;
	case VIA_MEM_SDR133:
		name = "SDR 133";
		break;
	case VIA_MEM_DDR_200:
		name = "DDR 200";
		break;
	case VIA_MEM_DDR_266:
		name = "DDR 266";
		break;
	case VIA_MEM_DDR_333:
		name = "DDR 333";
		break;
	case VIA_MEM_DDR_400:
		name = "DDR 400";
		break;
	case VIA_MEM_DDR2_400:
		name = "DDR2 400";
		break;
	case VIA_MEM_DDR2_533:
		name = "DDR2 533";
		break;
	case VIA_MEM_DDR2_667:
		name = "DDR2 667";
		break;
	case VIA_MEM_DDR2_800:
		name = "DDR2 800";
		break;
	case VIA_MEM_DDR2_1066:
		name = "DDR2 1066";
		break;
	default:
		break;
	}

	ret = ttm_bo_init_mm(&dev_priv->bdev, TTM_PL_VRAM, vram_size >> PAGE_SHIFT);
	if (!ret) {
		DRM_INFO("Detected %llu MB of %s Video RAM at physical address 0x%08llx.\n",
			(unsigned long long) vram_size >> 20, name,
			(unsigned long long) dev_priv->vram_start);
	}
out_err:
	if (bridge)
		pci_dev_put(bridge);
	if (fn3)
		pci_dev_put(fn3);
	return ret;
}
