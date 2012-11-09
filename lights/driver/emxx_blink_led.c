/*
 *  * FAN5646 blink led driver
 *   *
 *    * Copyright (C) 2011 Topwise Broncho Team
 *     *
 *      * This program is free software; you can redistribute it and/or modify
 *       * it under the terms of the GNU General Public License version 2 as
 *        * published by the Free Software Foundation.
 *         *
 *          */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/types.h>

#include <mach/gpio.h>

#define RED  0
#define BLUE 1

static int FAN5646_register[][5] = {// mode of the blinkled  
	{
		0x00,//T(RISE1)=0ms,T(FALL1)=0ms
		0x31,//T(ON1)=320ms,T(OFF1)=320ms
		0x00,//T(RISE2)=0ms,T(FALL2)=0ms
		0x31,//T(ON2)=320ms,T(OFF2)=320ms
		0x02//PLAY mode
	},
	{
		0x00,//T(RISE1)=0ms,T(FALL1)=0ms
		0x31,//T(ON1)=640ms,T(OFF1)=640ms
		0x00,//T(RISE2)=0ms,T(FALL2)=0ms
		0x31,//T(ON2)=640ms,T(OFF2)=640ms
		0x06//SLOW mode
	},
	{
		0x00,
		0xF0,//T(ON1)=1599ms,T(OFF1)=0ms
		0x00,
		0xF0,//T(ON2)=1599ms,T(OFF2)=0ms
		0x02//PLAY mode
	},
	{
		0x00,
		0x0F,//T(ON1)=0ms,T(OFF1)=1599ms
		0x00,
		0x0F,//T(ON2)=0ms,T(OFF2)=1599ms
		0x02//PLAY mode
	}
};

static int reg_add[5] = { // register address
	0,//SLEW1 address
	1,//PULSE1 address
	2,//SLEW2 address
	3,//PULSE2 address
	4,//CONTROL address
};

static int bin_8_bit[8] = {0,0,0,0,0,0,0,0};//binary array for register value
static int bin_3_bit[3] = {0,0,0}; //binary array for register address

static void hex_to_bin_data(int data)//register value changed to 8 bit
{
	int i = 0;
	for(; i<8; i++) {
		bin_8_bit[7-i] = data%2;
		data /= 2;
	}
}

static void hex_to_bin_add(int add)//register address changed to 3 bit
{
	int i = 0;
	for(; i<3; i++) {
		bin_3_bit[2-i] = add%2;
		add /= 2;
	}
}

static int blink_led_transmit_bit(int color, int data)//GPIO output 
{
	int ret;
	if(color == RED) {
		ret = gpio_direction_output(GPIO_BLINKLED_RED, data);
	} else if(color == BLUE) {
		ret = gpio_direction_output(GPIO_BLINKLED_GREEN, data);
	} else {
		return -1;
	}
	return ret;
}

static void check(int ret)//check the return error
{
	if(ret < 0)
		printk(KERN_INFO "#########fan5646 blink led driver: error %d\n", ret);
}

static int tinywire_protocol(int mode, int color)//write the FAN5646 register based on tinywire protocol
{
	int ret = 0;
	int counter =0;
	int temp;

	while(counter < 5) {
		hex_to_bin_add(reg_add[counter]);
		hex_to_bin_data(FAN5646_register[mode][counter]);

		//tinywire  protocol timing, LSB first, address first
		for(temp=2; temp>=0; temp--) {//address
			f(bin_3_bit[temp] == 1) {//rising edge always at first
				//rising edge: T(1):75%, T(0):25%  BIT=1
				ret = blink_led_transmit_bit(color, 1);
				check(ret);
				ndelay(1500);

				ret = blink_led_transmit_bit(color, 0);
				check(ret);
				ndelay(500);
			} else {//T(1):25%, T(0):75% BIT=0
				ret = blink_led_transmit_bit(color, 1);
				check(ret);
				ndelay(500);

				ret = blink_led_transmit_bit(color, 0);
				check(ret);
				ndelay(1500);
			}
		}

		for(temp=7; temp>=0; temp--) {//register data

			if(bin_8_bit[temp] == 1) {//rising edge: T(1):75%, T(0):25%  BIT=1
				ret = blink_led_transmit_bit(color, 1);
				check(ret);
				ndelay(1500);

				ret = blink_led_transmit_bit(color, 0);
				check(ret);
				ndelay(500);
			} else {//T(1):25%, T(0):75% BIT=0
				ret = blink_led_transmit_bit(color, 1);
				check(ret);

				ndelay(500);

				ret = blink_led_transmit_bit(color, 0);
				check(ret);
				ndelay(1500);
			}//end else
		}//end for

		//stop bit  
		blink_led_transmit_bit(color, 1);
		ndelay(500);//stop, T(high)
		blink_led_transmit_bit(color, 0);
		udelay(3);//stop, T(low)   rising within 4us of the stop bit's failing edge 

		counter++;
	}

	return ret;
}

static ssize_t device_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t device_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char cmd[20] = {0};

	if (sscanf(buf, "%s", cmd) == 1) {
		if(strcmp(cmd, "off") == 0 ) {// turn off both
			//printk("%s\n", cmd);  
			tinywire_protocol(3, RED);
			blink_led_transmit_bit(RED, 0);

			tinywire_protocol(3, BLUE);
			blink_led_transmit_bit(BLUE, 0);

		} else if(strcmp(cmd, "red_off") == 0 ) {// turn off red
			//printk("%s\n", cmd);  
			tinywire_protocol(3, RED);
			blink_led_transmit_bit(RED, 0);

		} else if(strcmp(cmd, "blue_off") == 0 ) {// turn off blue
			//printk("%s\n", cmd);  
			tinywire_protocol(3, BLUE);
			blink_led_transmit_bit(BLUE, 0);

		} else if(strcmp(cmd, "blue_blink") == 0) {//red blue mode 0
			//printk("%s\n", cmd);  
			tinywire_protocol(0, RED);
			blink_led_transmit_bit(RED, 1);
		} else if(strcmp(cmd, "blue_blink") == 0) {//blue blink mode 0
			//printk("%s\n", cmd);  
			tinywire_protocol(0, BLUE);
			blink_led_transmit_bit(BLUE, 1);

		} else if(strcmp(cmd, "red_blink_slow") == 0) {//red blink mode 1
			//printk("%s\n", cmd);  
			tinywire_protocol(1, RED);
			blink_led_transmit_bit(RED, 1);

		} else if(strcmp(cmd, "blue_blink_slow") == 0) {//blue blink mode 1
			//printk("%s\n", cmd);  
			tinywire_protocol(1, BLUE);
			blink_led_transmit_bit(BLUE, 1);

		} else if(strcmp(cmd, "red_on") == 0) {//turn on red
			//printk("%s\n", cmd);  
			tinywire_protocol(2, RED);
			blink_led_transmit_bit(RED, 1);

		} else if(strcmp(cmd, "blue_on") == 0) {//turn on blue
			//printk("%s\n", cmd);  
			tinywire_protocol(2, BLUE);
			blink_led_transmit_bit(BLUE, 1);

		} else if(strcmp(cmd, "purple_once") == 0) {//blink purple led once
			//printk("%s\n", cmd);  
			tinywire_protocol(2, RED);
			blink_led_transmit_bit(RED, 1);     //turn on red
			msleep(1000);
			tinywire_protocol(3, RED);
			blink_led_transmit_bit(RED, 0);     //turn off red
			msleep(1000);

			tinywire_protocol(2, BLUE);
			blink_led_transmit_bit(BLUE, 1);    //turn on blue 
			msleep(1000);

			tinywire_protocol(3, BLUE);
			blink_led_transmit_bit(BLUE, 0);    //turn off blue
			msleep(1000);
		}
	}

	return size;
}


static dev_t devno;
static struct class *class;
static struct device *device;
static DEVICE_ATTR(led, S_IRUGO | S_IWUSR, device_show, device_store);

static int create_blinkled_device(void)
{
	int ret = 0;

	class = class_create(THIS_MODULE, "emxx_led");
	if (IS_ERR(class))
	{
		ret = PTR_ERR(class);
		goto err_class_create;
	}

	device = device_create(class, NULL, devno, NULL, "%s", "emxx_blink_led");
	if (IS_ERR (device))
	{
		ret = PTR_ERR (device);
		goto err_device_create;
	}

	ret = device_create_file(device, &dev_attr_led);
	if (ret)
	{
		goto err_device_create_file;
	}

	return 0;

err_device_create_file:
	device_destroy(class, devno);

err_device_create:
	class_destroy(class);

err_class_create:
	return ret;
}

static void destroy_blinkled_device(void)
{
	device_remove_file(device, &dev_attr_led);
	device_destroy(class, devno);
	class_destroy(class);
}

static int blink_led_probe(struct platform_device *pdev)
{
	int ret = 0;

	if(pdev->id == 0) {
		ret = gpio_request(GPIO_BLINKLED_RED, "emxx-gpio-blink");
		if(ret < 0) {
			printk("%s : failed to request GPIO %d,error %d\n", __FUNCTION__, GPIO_BLINKLED_RED, ret);
			return ret;
		}
		emxx_single_mfp_config(MFP_CFG(GPIO_BLINKLED_RED, MFP_FUN_GPIO|MFP_PULL_DOWN|MFP_INPUT_NORMAL));
	} else {
		ret = gpio_request(GPIO_BLINKLED_GREEN, "emxx-gpio-blink");
		if(ret < 0) {
			printk("%s : failed to request GPIO %d,error %d\n", __FUNCTION__, GPIO_BLINKLED_GREEN, ret);
			return ret;
		}
		emxx_single_mfp_config(MFP_CFG(GPIO_BLINKLED_GREEN, MFP_FUN_GPIO|MFP_PULL_DOWN|MFP_INPUT_NORMAL));
	}


	return 0;
}

static int blink_led_remove(struct platform_device *pdev)
{
	gpio_free(GPIO_BLINKLED_GREEN);
	gpio_free(GPIO_BLINKLED_RED);
	destroy_blinkled_device();
	return 0;
}

static struct platform_driver blink_led_driver = {
	.probe = blink_led_probe,
	.remove = blink_led_remove,
	.driver = {
		.name   = "emxx-gpio-led",
		.owner  = THIS_MODULE,
	},
};
static int __init blink_led_init(void)
{
	int ret = 0;

	if(create_blinkled_device()) {
		ret = -ENOMEM;
		goto err_create_blinkled_device;
	}

	ret = platform_driver_register(&blink_led_driver);
	if(ret){
		goto err_platform_driver_register;
	}
	return 0;
err_platform_driver_register:
	destroy_blinkled_device();

err_create_blinkled_device:
	return ret;
}

static void __exit blink_led_exit(void)
{
	platform_driver_unregister(&blink_led_driver);
}

module_exit(blink_led_exit);
module_init(blink_led_init);

MODULE_DESCRIPTION("FAN5646 blink led driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xiongyupeng <xiongyupeng@topwise3g.com>");
