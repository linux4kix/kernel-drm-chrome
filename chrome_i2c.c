/*
 * via I2C
 */

#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include "drmP.h"
#include "drm.h"
#include "drm_crtc.h"
#include "drm_crtc_helper.h"

#include "crtc_hw.h"
#include "via_drv.h"

struct via_i2c_adap {
	struct i2c_algo_bit_data algo;
	struct i2c_adapter adapter;
	void __iomem *port;
	unsigned int index;
	bool gpio;
};

static struct via_i2c_adap via_gpio[5];

/*[VIA_PORT_26]   = { VIA_PORT_I2C,  VIA_MODE_I2C, VIASR, 0x26 },
[VIA_PORT_31]   = { VIA_PORT_I2C,  VIA_MODE_I2C, VIASR, 0x31 },
[VIA_PORT_25]   = { VIA_PORT_GPIO, VIA_MODE_GPIO, VIASR, 0x25 },
[VIA_PORT_2C]   = { VIA_PORT_GPIO, VIA_MODE_I2C, VIASR, 0x2c },
[VIA_PORT_3D]   = { VIA_PORT_GPIO, VIA_MODE_GPIO, VIASR, 0x3d },
*/

static void
via_setscl(void *data, int state)
{
	struct via_i2c_adap *adap_data = data;
	u8 val;

	iowrite8(adap_data->index, adap_data->port);
	val = ioread8(adap_data->port + 1) & 0xf0;	

	if (state)
		val |= 0x20;
	else
		val &= ~0x20;

	if (adap_data->gpio)
		val |= 0x80;
	else
		val |= 0x01;

	iowrite8(adap_data->index, adap_data->port);
	iowrite8(val, adap_data->port + 1);
}

static int 
via_getscl(void *data)
{
	struct via_i2c_adap *adap_data = data;

	iowrite8(adap_data->index, adap_data->port);
        if (ioread8(adap_data->port + 1) & 0x08)
		return 1;
	return 0;
}

static void
via_setsda(void *data, int state)
{
	struct via_i2c_adap *adap_data = data;
	u8 val;

	iowrite8(adap_data->index, adap_data->port);
	val = ioread8(adap_data->port + 1) & 0xf0;	

	if (state)
		val |= 0x10;
	else
		val &= ~0x10;

	if (adap_data->gpio)
		val |= 0x40;
	else
		val |= 0x01;

	iowrite8(adap_data->index, adap_data->port);
	iowrite8(val, adap_data->port + 1);
}

static int 
via_getsda(void *data)
{
	struct via_i2c_adap *adap_data = data;

	iowrite8(adap_data->index, adap_data->port);
        if (ioread8(adap_data->port + 1) & 0x04)
		return 1;
	return 0;
}

static int
via_setup_i2c_bus(struct via_i2c_adap *adap, const char *name,
			struct drm_device *dev)
{
	int rc;

	strlcpy(adap->adapter.name, name, sizeof(adap->adapter.name));
	adap->adapter.owner		= THIS_MODULE;
	adap->adapter.algo_data		= &adap->algo;
	adap->adapter.dev.parent	= &dev->pdev->dev;
	adap->algo.setsda		= via_setsda;
	adap->algo.setscl		= via_setscl;
	adap->algo.getsda		= via_getsda;
	adap->algo.getscl		= via_getscl;
	adap->algo.udelay		= 10;
	adap->algo.timeout		= msecs_to_jiffies(500);
	adap->algo.data 		= adap;

	i2c_set_adapdata(&adap->adapter, adap);

	rc = i2c_bit_add_bus(&adap->adapter);
	if (rc == 0)
		printk(KERN_INFO "I2C bus %s registered.\n", name);
	return rc;
}

int 
via_encoder_probe(struct drm_device *dev, int encoder, void __iomem *iobase,
			unsigned int index)
{
	struct via_i2c_adap *adap = &via_gpio[encoder];
	int ret = 0;

	adap->port = iobase;
	adap->index = index;
	adap->gpio = false;
	ret = via_setup_i2c_bus(adap, "VIA I2C CRT", dev);
	return ret;
}

void
via_encoder_destroy(struct drm_device *dev, int connector_type)
{
	struct drm_via_private *dev_priv = dev->dev_private;
	struct i2c_adapter *adapter = NULL;

	if (connector_type == DRM_MODE_CONNECTOR_VGA)
		adapter = &via_gpio[0].adapter;

	if (adapter)
		i2c_del_adapter(adapter);
}

int via_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_via_private *dev_priv = dev->dev_private;
	struct i2c_adapter *adapter = NULL;
	struct edid *edid;
	int ret = 0;

	if (connector->connector_type == DRM_MODE_CONNECTOR_VGA)
		adapter = &via_gpio[0].adapter;

	if (adapter) {
		edid = drm_get_edid(connector, adapter);
		if (edid) {
			DRM_INFO("edid is present\n");
			drm_mode_connector_update_edid_property(connector, edid);
			ret = drm_add_edid_modes(connector, edid);
			kfree(edid);
		}
	}
	return ret;
}
