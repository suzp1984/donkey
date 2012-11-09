/* drivers/leds/leds-broncho.c
 *
 ** Copyright (C) 2009 Broncho Team
 * *
 *  * Author: Mai ZhuRong <qjacket@163.com>
 *   *
 *    * This software is licensed under the terms of the GNU General Public
 *     * License version 2, as published by the Free Software Foundation, and
 *      * may be copied, distributed, and modified under those terms.
 *       *
 *        * This program is distributed in the hope that it will be useful,
 *         * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *          * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *           * GNU General Public License for more details.
 *            *
 *             */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/leds.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/micco.h>
#include <mach/mfp-pxa3xx.h>
#include <mach/mfp.h>
#include <mach/ipmc.h>
#include <mach/gpio.h>

#include <mach/littleton.h>
#include <linux/clk.h>

#define PWMCR       (0x00)
#define PWMDCR      (0x04)
#define PWMPCR      (0x08)

#define JOGBALL_LED_MAX (0X3FF)
#define JOGBALL_LED_MIN (0X0)

typedef struct _LedData
{
	spinlock_t data_lock;
	struct led_classdev leds[4];
	int gpio_ctl1;
	int gpio_ctl2;
	struct clk *clk;
	u32 memio_base;
}LedData;

static void micco_led_pc_power (int on)
{
	if (on)
	{
		micco_write (MICCO_LEDPC_CONTROL1, 0xff);
		micco_write (MICCO_LEDPC_CONTROL2, 0xff);
		micco_write (MICCO_LEDPC_CONTROL3, 0xff);
		micco_write (MICCO_LEDPC_CONTROL4, 0xff);
		micco_write (MICCO_LEDPC_CONTROL5, 0x8);
	}
	else
	{
		micco_write (MICCO_LEDPC_CONTROL1, 0x0);
		micco_write (MICCO_LEDPC_CONTROL2, 0x0);
		micco_write (MICCO_LEDPC_CONTROL3, 0x0);
		micco_write (MICCO_LEDPC_CONTROL4, 0x0);
		micco_write (MICCO_LEDPC_CONTROL5, 0x0);
		pxa3xx_mfp_set_lpm (MFP_CFG_PIN (MFP_LED_CTL1), MFP_LPM_FLOAT);
		pxa3xx_mfp_set_lpm (MFP_CFG_PIN (MFP_LED_CTL2), MFP_LPM_FLOAT);
	}

	return;
}
static void set_brightness_red (LedData *broncho_led, enum led_brightness brightness)
{
	if (brightness == LED_OFF)
	{
		micco_led_pc_power (LED_OFF);
	}
	else
	{
		gpio_direction_output (broncho_led->gpio_ctl1, 1);
		pxa3xx_mfp_set_lpm (MFP_CFG_PIN (MFP_LED_CTL1), MFP_LPM_PULL_HIGH);
		gpio_direction_output (broncho_led->gpio_ctl2, 1);
		pxa3xx_mfp_set_lpm (MFP_CFG_PIN (MFP_LED_CTL2), MFP_LPM_PULL_HIGH);
		micco_led_pc_power (LED_FULL);
	}

	return;
}

static void set_brightness_green (LedData *broncho_led, enum led_brightness brightness)
{
	if (brightness == LED_OFF)
	{
		micco_led_pc_power (LED_OFF);
	}
	else
	{
		gpio_direction_output (broncho_led->gpio_ctl1, 1);
		pxa3xx_mfp_set_lpm (MFP_CFG_PIN (MFP_LED_CTL1), MFP_LPM_PULL_HIGH);
		gpio_direction_output (broncho_led->gpio_ctl2, 0);
		pxa3xx_mfp_set_lpm (MFP_CFG_PIN (MFP_LED_CTL2), MFP_LPM_PULL_LOW);
		micco_led_pc_power (LED_FULL);
	}

	return;
}

static void set_brightness_blue (LedData *broncho_led, enum led_brightness brightness)
{
	if (brightness == LED_OFF)
	{
		micco_led_pc_power (LED_OFF);
	}
	else
	{
		gpio_direction_output (broncho_led->gpio_ctl1, 0);
		pxa3xx_mfp_set_lpm (MFP_CFG_PIN (MFP_LED_CTL1), MFP_LPM_PULL_LOW);
		gpio_direction_output (broncho_led->gpio_ctl2, 0);
		pxa3xx_mfp_set_lpm (MFP_CFG_PIN (MFP_LED_CTL2), MFP_LPM_PULL_LOW);
		micco_led_pc_power (LED_FULL);
	}

	return;
}

static void set_brightness_jogball_backlight (LedData *broncho_led, enum led_brightness brightness)
{
	if (brightness == LED_OFF)
	{
		__raw_writel (0x0, broncho_led->memio_base + PWMCR);
		__raw_writel (0x0, broncho_led->memio_base + PWMDCR);
		__raw_writel (0x0, broncho_led->memio_base + PWMPCR);
		clk_disable (broncho_led->clk);
	}
	else
	{
		brightness = (brightness * JOGBALL_LED_MAX) / LED_FULL;
		clk_enable (broncho_led->clk);
		__raw_writel (0x08, broncho_led->memio_base + PWMCR);
		__raw_writel (brightness, broncho_led->memio_base + PWMDCR);
		__raw_writel (0x3ff, broncho_led->memio_base + PWMPCR);
	}

	return;
}


static void broncho_led_set_brightness (LedData *broncho_led, struct led_classdev *led, enum led_brightness brightness)
{
	if (!strcmp (led->name, "red"))
	{
		set_brightness_red (broncho_led, brightness);
	}
	else if (!strcmp (led->name, "green"))
	{
		set_brightness_green (broncho_led, brightness);
	}
	else if (!strcmp (led->name, "blue"))
	{
		set_brightness_blue (broncho_led, brightness);
	}
	else if (!strcmp (led->name, "jogball-backlight"))
	{
		set_brightness_jogball_backlight (broncho_led, brightness);
	}
	else
	{
		printk (KERN_ERR"%s: no this led\n", __func__);
	}

	return;
}

static int led_blink_set (struct led_classdev *led_cdev, unsigned long *delay_on, unsigned long *delay_off)
{
	unsigned long on = *delay_on;
	unsigned long off = *delay_off;
	u8 ledpc5 = 0;
	u32 ledpc_1_4 = 0;
	int i = 0;

	off = (off > 7000) ? 7000 : off;
	on = (on > 1280) ? 1280 : on;

	for (i=0; i<on/40; i++)
	{
		ledpc_1_4 |= (1<<i);
	}

	micco_write (MICCO_LEDPC_CONTROL1, 0x0);
	micco_write (MICCO_LEDPC_CONTROL2, 0x0);
	micco_write (MICCO_LEDPC_CONTROL3, 0x0);
	micco_write (MICCO_LEDPC_CONTROL4, 0x0);

	micco_write (MICCO_LEDPC_CONTROL1, ((ledpc_1_4 >> 0) & 0xff));
	micco_write (MICCO_LEDPC_CONTROL2, ((ledpc_1_4 >> 8) & 0xff));
	micco_write (MICCO_LEDPC_CONTROL3, ((ledpc_1_4 >> 16) & 0xff));
	micco_write (MICCO_LEDPC_CONTROL4, ((ledpc_1_4 >> 24) & 0xff));
	micco_read (MICCO_LEDPC_CONTROL5, &ledpc5);
	ledpc5 &= ~0x7;
	micco_write (MICCO_LEDPC_CONTROL5, ledpc5 | ((off/1000) & 0x7));

	*delay_on = on;
	*delay_off = off;

	return 0;
}

static void led_brightness_set (struct led_classdev *led_cdev, enum led_brightness brightness)
{
	LedData *broncho_led =  NULL;
	int idx = 0;

	if (!strcmp (led_cdev->name, "red"))
	{
		idx = 0;
	}
	else if (!strcmp (led_cdev->name, "green"))
	{
		idx = 1;
	}
	else if (!strcmp (led_cdev->name, "blue"))
	{
		idx = 2;
	}
	else if (!strcmp (led_cdev->name, "jogball-backlight"))
	{
		idx = 3;
	}
	else
	{
		printk (KERN_ERR"%s: no this led\n", __func__);
		return;
	}

	broncho_led = container_of (led_cdev, LedData, leds[idx]);

	spin_lock (&broncho_led->data_lock);
	broncho_led_set_brightness (broncho_led, led_cdev, brightness);
	spin_unlock (&broncho_led->data_lock);

	return;
}

static int broncho_led_probe (struct platform_device *pdev)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	struct resource *res = NULL;
	LedData *broncho_led = NULL;

	broncho_led = kzalloc (sizeof (LedData), GFP_KERNEL);
	if (broncho_led == NULL)
	{
		printk(KERN_ERR "broncho_led_probe: no memory for device\n");
		return -ENOMEM;
	}

	memset (broncho_led, 0, sizeof (LedData));

	broncho_led->clk = clk_get (NULL, "PWM1CLK");
	if (broncho_led->clk == NULL)
	{
		printk (KERN_ERR"%s: clk_get failed\n", __func__);
		ret = -ENOMEM;
		goto err_alloc_failed;
	}

	broncho_led->gpio_ctl1 = mfp_to_gpio (MFP_CFG_PIN (MFP_LED_CTL1));
	if (gpio_request (broncho_led->gpio_ctl1, "LED_PC Ctrl1"))
	{
		printk (KERN_ERR"%s: failed to request gpio LED_PC Ctrl1\n", __func__);
		ret = -EINVAL;
		goto err_request_gpio_ctl1;
	}

	broncho_led->gpio_ctl2 =  mfp_to_gpio (MFP_CFG_PIN (MFP_LED_CTL2));
	if (gpio_request (broncho_led->gpio_ctl2, "LED_PC Ctrl2"))
	{
		printk (KERN_ERR"%s: failed to request gpio LED_PC Ctrl2\n", __func__);
		ret = -EINVAL;
		goto err_request_gpio_ctl2;
	}

	res = platform_get_resource (pdev, IORESOURCE_IO, 0);
	if (!res)
	{
		printk (KERN_ERR"%s: failed to get resource\n", __func__);
		ret = -ENOMEM;
		goto err_get_resource;
	}

	broncho_led->memio_base = (u32)ioremap (res->start, res->end - res->start + 1);

	broncho_led->leds[0].name = "red";
	broncho_led->leds[0].brightness_set = led_brightness_set;
	broncho_led->leds[0].blink_set = led_blink_set;

	broncho_led->leds[1].name = "green";
	broncho_led->leds[1].brightness_set = led_brightness_set;
	broncho_led->leds[1].blink_set = led_blink_set;

	broncho_led->leds[2].name = "blue";
	broncho_led->leds[2].brightness_set = led_brightness_set;
	broncho_led->leds[2].blink_set = led_blink_set;

	broncho_led->leds[3].name = "jogball-backlight";
	broncho_led->leds[3].brightness_set = led_brightness_set;

	spin_lock_init (&broncho_led->data_lock);

	for (i = 0; i < 4; i++)
	{
		ret = led_classdev_register (&pdev->dev, &broncho_led->leds[i]);
		if (ret)
		{
			printk (KERN_ERR"broncho_led: led_classdev_register failed\n");
			goto err_led_classdev_register_failed;
		}
	}

	dev_set_drvdata (&pdev->dev, broncho_led);

	micco_led_pc_power (LED_OFF);
	set_brightness_jogball_backlight (broncho_led, LED_OFF);

	return 0;

err_led_classdev_register_failed:
	for (j = 0; j < i; j++)
	{
		led_classdev_unregister (&broncho_led->leds[j]);
	}

err_get_resource:
	gpio_free (broncho_led->gpio_ctl2);

err_request_gpio_ctl2:
	gpio_free (broncho_led->gpio_ctl1);

err_request_gpio_ctl1:
	clk_put (broncho_led->clk);

err_alloc_failed:
	kfree (broncho_led);

	return ret;
}

static int __devexit broncho_led_remove (struct platform_device *pdev)
{
	LedData *broncho_led = platform_get_drvdata (pdev);
	int i = 0;

	for (i=0; i<4; i++)
	{
		led_classdev_unregister (&broncho_led->leds[i]);
	}
	gpio_free (broncho_led->gpio_ctl2);
	gpio_free (broncho_led->gpio_ctl1);
	clk_put (broncho_led->clk);
	kfree (broncho_led);

	return 0;
}

static struct platform_driver broncho_led_driver =
{
	.probe = broncho_led_probe,
	.remove = __devexit_p (broncho_led_remove),
	.driver =
	{
		.name = "micco-leds",
		.owner = THIS_MODULE,
	},
};

static int __init broncho_led_init (void)
{
	return platform_driver_register (&broncho_led_driver);
}
static void __exit broncho_led_exit (void)
{
	platform_driver_unregister (&broncho_led_driver);
}

MODULE_AUTHOR ("Mai ZhuRong <qjacket@163.com>");
MODULE_DESCRIPTION ("Broncho LEDs");
MODULE_LICENSE ("GPL");

module_init (broncho_led_init);
module_exit (broncho_led_exit);

