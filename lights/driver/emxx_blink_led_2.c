/*
 * FAN5646 blink led driver
 *
 * Copyright (C) 2011 Topwise Broncho Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

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


#define BLINK_LED_MAJOR	100	//需要确认此主设备号是否可使用

static int FAN5646_register[][5] = {// mode of the blink led  
	{//led always on
		0x00,//T(RISE1)=0ms,T(FALL1)=0ms
		0xF0,//T(ON1)=1599ms,T(OFF1)=0ms
		0x00,//T(RISE1)=0ms,T(FALL1)=0ms
		0xF0,//T(ON2)=1599ms,T(OFF2)=0ms
		0x02//PLAY mode
	},
	{//led always off
		0x00,//T(RISE1)=0ms,T(FALL1)=0ms
		0x0F,//T(ON1)=0ms,T(OFF1)=1599ms
		0x00,//T(RISE1)=0ms,T(FALL1)=0ms
		0x0F,//T(ON2)=0ms,T(OFF2)=1599ms
		0x02//PLAY mode
	},
	{//led blink
		0x22,//T(RISE1)=206ms,T(FALL1)=206ms
		0x31,//T(ON1)=320ms,T(OFF1)=320ms
		0x22,//T(RISE2)=206ms,T(FALL2)=206ms
		0x31,//T(ON2)=320ms,T(OFF2)=320ms
		0x02//PLAY mode
	},
	{//led blink more slowly
		0x22,//T(RISE1)=206ms,T(FALL1)=206ms
		0x31,//T(ON1)=640ms,T(OFF1)=640ms
		0x22,//T(RISE2)=206ms,T(FALL2)=206ms
		0x31,//T(ON2)=640ms,T(OFF2)=640ms
		0x06//SLOW mode
	}
};

static int reg_add[5] = {// register address
	0,//SLEW1 address
	1,//PULSE1 address
	2,//SLEW2 address
	3,//PULSE2 address
	4,//CONTROL address
};

struct blink_led_t{
	int id;
	int gpio;
	dev_t devno;
	char name[20];
	struct device *device;
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

static int tinywire_protocol(struct blink_led_t *led, int mode, int data)
{
	int ret = 0;
	int counter =0;
	int temp;

printk("gpio %d\n", led->gpio);

	while(counter < 5) {
		hex_to_bin_add(reg_add[counter]);
		hex_to_bin_data(FAN5646_register[mode][counter]);

		//tinywire  protocol timing, LSB first, address first
		for(temp=2; temp>=0; temp--) {//address
			if(bin_3_bit[temp] == 1) {//rising edge always at first
				//rising edge: T(1):75%, T(0):25%  BIT=1
				gpio_direction_output(led->gpio, 1);
				ndelay(1500);

				gpio_direction_output(led->gpio, 0);
				ndelay(500);
			} else {//T(1):25%, T(0):75% BIT=0
				gpio_direction_output(led->gpio, 1);
				ndelay(500);

				gpio_direction_output(led->gpio, 0);
				ndelay(1500);
			}//end else
		}//end for

		for(temp=7; temp>=0; temp--) {//register data
			if(bin_8_bit[temp] == 1) {//rising edge: T(1):75%, T(0):25%  BIT=1
				gpio_direction_output(led->gpio, 1);
				ndelay(1500);

				gpio_direction_output(led->gpio, 0);
				ndelay(500);
			} else {//T(1):25%, T(0):75% BIT=0
				ret = gpio_direction_output(led->gpio, 1);
				ndelay(500);

				gpio_direction_output(led->gpio, 0);
				ndelay(1500);
			}//end else
		}//end for

		//stop bit  
		gpio_direction_output(led->gpio, 1);
		ndelay(500);//stop, T(high)
		gpio_direction_output(led->gpio, 0);
		udelay(3);//stop, T(low)   rising within 4us of the stop bit's failing edge 

		counter++;
	}//end while

	gpio_direction_output(led->gpio, data);

	return 0;
}

static ssize_t device_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return 0;
}

static ssize_t device_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char cmd[20] = {0};
	struct blink_led_t *led;

	led = (struct blink_led_t *)dev_get_drvdata(dev);

	if (sscanf(buf, "%s", cmd) == 1) {
		if(strcmp(cmd, "on") == 0 ) {//mode 0: turn on led
			printk("led%d: %s\n", led->id, cmd);  
			tinywire_protocol(led, 0, 1);

		} else if(strcmp(cmd, "off") == 0 ) {//mode 1: turn off led
			printk("led%d: %s\n", led->id, cmd);  
			tinywire_protocol(led, 1, 0);

		} else if(strcmp(cmd, "blink") == 0 ) {//mode 1: blink led
			printk("led%d: %s\n", led->id, cmd);  
			tinywire_protocol(led, 2, 1);

		} else if(strcmp(cmd, "blink_slow") == 0) {//mode 3: blink led slowly
			printk("led%d: %s\n", led->id, cmd);  
			tinywire_protocol(led, 3, 1);
		}
	}

	return size;
}

static struct class *class;
static DEVICE_ATTR(led, S_IRUGO | S_IWUSR, device_show, device_store);


static int blink_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct blink_led_t *blink_led;

	blink_led = kzalloc(sizeof(struct blink_led_t), GFP_KERNEL);
        if(NULL == blink_led){
		printk(KERN_INFO "Failed to request memory for blink led\n");
		ret = -ENOMEM;
		goto err_kzalloc;
	}

	blink_led->id = pdev->id;
	blink_led->gpio = (int)pdev->dev.driver_data;
	sprintf(blink_led->name, "%s%d", pdev->name, pdev->id);
	blink_led->devno = MKDEV(BLINK_LED_MAJOR, pdev->id);

	ret = gpio_request(blink_led->gpio, blink_led->name);
        if(ret < 0){
		printk(KERN_INFO "Failed to request GPIO %d, error %d\n", blink_led->gpio, ret);
		goto err_gpio_request;
	}
	emxx_single_mfp_config(MFP_CFG(blink_led->gpio, MFP_FUN_GPIO|MFP_PULL_DOWN|MFP_INPUT_NORMAL));

	blink_led->device = device_create(class, NULL, blink_led->devno, NULL, "%s", blink_led->name);
	if (IS_ERR(blink_led->device)){
		ret = PTR_ERR(blink_led->device);
		goto err_device_create;
	}

	platform_set_drvdata(pdev, blink_led);
	dev_set_drvdata(blink_led->device, blink_led);

	ret = device_create_file(blink_led->device, &dev_attr_led);
	if(ret){
		goto err_device_create_file;
	}

	printk("************* blink led%d driver probe done ****************\n", blink_led->id);

	return 0;

err_device_create_file:
	device_destroy(class, blink_led->devno);

err_device_create:
	gpio_free(blink_led->gpio);

err_gpio_request:
	kfree(blink_led);

err_kzalloc:
	return ret;
}

static int blink_led_remove(struct platform_device *pdev)
{
	struct blink_led_t *led = (struct blink_led_t *)platform_get_drvdata(pdev);

	device_destroy(class, led->devno);
	gpio_free(led->gpio);
	kfree(led);

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
	
	printk("************* %s ****************\n", __FUNCTION__);
	
	class = class_create(THIS_MODULE, "emxx_led");
	if (IS_ERR(class)){
		printk(KERN_INFO "Failed to create emxx_led class.\n");
		ret = PTR_ERR(class);
		goto err_class_create;
	}

	ret = platform_driver_register(&blink_led_driver);
	if(ret){
		printk(KERN_INFO "Failed to register platform led driver.\n");
		goto err_platform_driver_register;
	}

	return 0;

err_platform_driver_register:
	class_destroy(class);

err_class_create:
	return ret;
}

static void __exit blink_led_exit(void)
{
	class_destroy(class);
	platform_driver_unregister(&blink_led_driver);
}

module_exit(blink_led_exit);
module_init(blink_led_init);

MODULE_DESCRIPTION("FAN5646 blink led driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xiongyupeng <xiongyupeng@topwise3g.com>");


