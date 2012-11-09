/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#define LOG_TAG "lights"
#define  LIGHTS_SERVICE_NAME "lights"

#include <cutils/log.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/input.h>
#include <sys/select.h>

#include <hardware/lights.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>

#define LCD_BACKLIGHT      "/sys/class/backlight/emxx_pwm_bl0"

#ifdef N5
#define BUTTON_BACKLIGHT "/sys/devices/virtual/backlight/emxx-gpio-bl0"
#endif

#ifdef N701
#define BUTTON_BACKLIGHT   "/sys/class/backlight/emxx-gpio-bl0"
#endif

#ifndef BUTTON_BACKLIGHT
#define BUTTON_BACKLIGHT   "/sys/class/backlight/emxx-pwm-bl1"
#endif

#define TRIGGER_BLINK      "timer"

/* N5 blink light */
#define GREEN_NOTIFICATION "/sys/bus/platform/devices/emxx-gpio-led.0/led"
#define BLUE_NOTIFICATION "/sys/bus/platform/devices/emxx-gpio-led.1/led"

#define LED_ON "on"
#define LED_OFF "off"
#define LED_BLINK "blink"
#define LED_BLINK_SLOW "blink_slow"

static int back_light_max = 0;
static int button_light_max = 0;

/******************************************************************************/

int back_light_set_light (struct light_device_t* dev, struct light_state_t const* state)
{
    int fd;

    if (back_light_max == 0)
    {
        fd = open(LCD_BACKLIGHT"/max_brightness", O_RDONLY);
        if (fd) { 
            char    buffer[20];
            if (read(fd, buffer, sizeof(buffer)) > 0)
            {
                back_light_max = atol(buffer);
            }
            close(fd);
        }

        back_light_max = back_light_max == 0 ? 127 : back_light_max;
    }

    unsigned int light = ((77*((state->color>>16)&0x00ff)) +
                          (150*((state->color>>8)&0x00ff)) +
                          (29*(state->color&0x00ff))) 
                          >> 8;

    //LOGE("set back light org %d\n", light);
    light = light * back_light_max / 255;
    //LOGE("set back light act %d\n", light);

    fd = open(LCD_BACKLIGHT"/brightness", O_WRONLY);
    if (fd) {
        char    buffer[20];
        int bytes = sprintf(buffer, "%u\n", light);
        write(fd, buffer, bytes);
        close(fd);
        
        return 0;

    } else {
        LOGE("set back light failed to open device\n");

        return -1;
    }
}


int buttons_light_set_light (struct light_device_t* dev, struct light_state_t const* state)
{
    int fd;

    if (button_light_max == 0)
    {
        fd = open(BUTTON_BACKLIGHT"/max_brightness", O_RDONLY);
        if (fd) { 
            char    buffer[20];
            if (read(fd, buffer, sizeof(buffer)) > 0)
            {
                button_light_max = atol(buffer);
            }
            close(fd);
        }

        button_light_max = button_light_max == 0 ? 127 : button_light_max;
    }

    unsigned int light = ((77*((state->color>>16)&0x00ff)) +
                          (150*((state->color>>8)&0x00ff)) +
                          (29*(state->color&0x00ff))) 
                          >> 8;

    //LOGE("set button light org %d\n", light);
    light = light * button_light_max / 255;
    //LOGE("set button light act %d\n", light);

    fd = open(BUTTON_BACKLIGHT"/brightness", O_WRONLY);
    if (fd) {
        char    buffer[20];
        int bytes = sprintf(buffer, "%u\n", light);
        write(fd, buffer, bytes);
        close(fd);
        
        return 0;

    } else {
        LOGE("set button light failed to open device\n");

        return -1;
    }
}

static void set_led_light (char* led_path, char* cmd) {
	char buffer[16];
	int fd;

	fd = open(led_path, O_WRONLY);
	if(fd >= 0) {
		//LOGE("+++++++++++++++++++write cmd to path-----------------: %s\n", cmd);
		write(fd, cmd, strlen(cmd));
		close(fd);
	} else {
		LOGE("cannot open notification red light: %s\n", led_path);
	}
}

int notifications_light_set_light (struct light_device_t* dev, struct light_state_t const* state)
{
    int red, green, blue;
    int flash_mode, on, off;
    char cmd_buffer[12];

    red   = (state->color >> 16) & 0xFF;
    green = (state->color >>  8) & 0xFF;
    blue  =  state->color & 0xFF;

    on = state->flashOnMS;
    off = state->flashOffMS;
    flash_mode = state->flashMode;
   	//LOGE("===========in notifications_light_set_light on is %d", on);
	//LOGE("==========in notifications_light_set_light off is %d", off);
	if(on > 0) {
		on = on/106.6;
		on++;
		if(on > 15) {
			on = 15;
		}
	} else {
		on = 0;
	}

	if(off > 0) {
		off = off/320;
		off++;
		if(off > 15) {
			off = 15;
		}
	} else {
		off = 0;
	}
	//LOGE("===========in notifications_light_set_light on is %d", on);
	//LOGE("==========in notifications_light_set_light off is %d", off);

	snprintf(cmd_buffer, 12, "0x0 0x%x 0x%x", on, off);
	//turn off light first
	//LOGE("++++++++++++++in notification set_light ");
	set_led_light(GREEN_NOTIFICATION, "0x0 0x0 0xf");
	set_led_light(BLUE_NOTIFICATION, "0x0 0x0 0xf");
	
	if(red > 0 || green > 0) {
		if(flash_mode == LIGHT_FLASH_TIMED) {
			//LOGE("+++++++++++in notification green LIGHT_FLASH_TIMED %s", cmd_buffer);
			set_led_light(GREEN_NOTIFICATION, cmd_buffer);
		} else if (flash_mode == LIGHT_FLASH_NONE) {
			//LOGE("+++++++++++in notification green LIGHT_FLASH_NONE ");
			set_led_light(GREEN_NOTIFICATION, cmd_buffer);
		} else if (flash_mode == LIGHT_FLASH_HARDWARE) {
			//LOGE("+++++++++++in notification green LIGHT_FLASH_HARDWARE");
		}
	} else if (blue > 0) {
		if(flash_mode == LIGHT_FLASH_TIMED) {
			//LOGE("+++++++++++in notfications blue  LIGHT_FLASH_TIMED %s", cmd_buffer);
			set_led_light(BLUE_NOTIFICATION, cmd_buffer);
		} else if (flash_mode == LIGHT_FLASH_NONE) {
			//LOGE("+++++++++++in notfications blue LIGHT_FLASH_NONE");
			set_led_light(BLUE_NOTIFICATION, cmd_buffer);
		} else if (flash_mode == LIGHT_FLASH_HARDWARE) {
			//LOGE("+++++++++++in notfications blue LIGHT_FLASH_HARDWARE ");
		}
	} 

    return 0;
}

static int close_lights(struct light_device_t* dev)
{
	if(dev) {
		free(dev);
	}
	return 0;
}

/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int (*set_light)(struct light_device_t* dev,
    		struct light_state_t const* state);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name)) {
    	set_light = back_light_set_light;
	} else if (0 == strcmp(LIGHT_ID_BUTTONS, name)) {
		set_light = buttons_light_set_light;
	} else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name)) {
		set_light = notifications_light_set_light;
	} else if (0 == strcmp(LIGHT_ID_ATTENTION, name)) {
		set_light = notifications_light_set_light;
	} else {
		return -EINVAL;
	}
	
	struct light_device_t* dev = malloc(sizeof(struct light_device_t));
	memset(dev, 0, sizeof(*dev));

	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (struct hw_module_t*)module;
	dev->common.close = (int (*)(struct hw_device_t*))close_lights;
	dev->set_light = set_light;

	*device = (struct hw_device_t*)dev;
	return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

/*
 * The Lights Hardware Module
 */
const struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Broncho lights Module",
    .author = "broncho",
    .methods = &lights_module_methods,
};
