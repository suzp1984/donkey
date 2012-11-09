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


#define LOG_TAG "sensors"
#define  SENSORS_SERVICE_NAME "sensors"

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

#include <hardware/sensors.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>

#include "uinput.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/******************************************************************************/
#define ID_BASE SENSORS_HANDLE_BASE
#define ID_ACCELERATION (ID_BASE+1)
#define ID_MAGNETIC_FIELD (ID_BASE+2)
#define ID_ORIENTATION (ID_BASE+3)

/*support g_sensor and magnetic. TODO: add orintation later*/
#define SENSORS_SUPPORT_COUNT 2
#define SENSORS_FD_COUNT 2

#define LSG                     (980.0f)
#define G_SENSOR_CONVERT                 0.00981f
#define M_SENSOR_CONVERT		0.1f

#define SENSORS_ACCELERATION    (1 << (ID_ACCELERATION - 1))
#define SENSORS_MAGNETIC_FIELD  (1 << (ID_MAGNETIC_FIELD - 1))
#define INPUT_DIR               "/dev/input"
#define SUPPORTED_SENSORS       ((1 << SENSORS_SUPPORT_COUNT) - 1)

#define EVENT_MASK_ACCEL_ALL    ( (1 << ABS_X) | (1 << ABS_Y) | (1 << ABS_Z))
#define DEFAULT_THRESHOLD 100

#define AXIS_X (1 << ABS_X)
#define AXIS_Y (1 << ABS_Y)
#define AXIS_Z (1 << ABS_Z)
#define SENSORS_AXIS_ALL (AXIS_X | AXIS_Y | \
                                  AXIS_Z)
#define SEC_TO_NSEC 1000000000LL
#define USEC_TO_NSEC 1000

/** zxsu: add G_SENSOR_NAME and M_SENSOR_NAME here **/
#define G_SENSOR_NAME "g_sensor_lis35de"
#define M_SENSOR_NAME "ms_hmcxx"

/** zxsu: notice add magnitic sensor stuff **/
#define G_SENSOR_DEVICE        "/sys/class/g_sensor/g_sensor_class/device" //TODO 
#define G_SENSOR_CMD_POWER_ON  "on"
#define G_SENSOR_CMD_POWER_OFF "off"
#define G_SENSOR_CMD_START_TIMER "start %u"
#define G_SENSOR_CMD_STOP_TIMER  "stop"

#define G_SENSOR_DEFAULT_TIMER 400

/** add mgenitic field sensor **/
#define M_SENSOR_DEVICE        "/sys/class/m_sensor/m_sensor_class/device"
#define M_SENSOR_CMD_START_RATE "start %u"
#define M_SENSOR_CMD_START "start"
#define M_SENSOR_CMD_STOP_RATE "stop"

/** supported frequency of m_sensor: 
 * 0x0 0.75hz  1333.33ms
 * 0x1 1.5hz	666.667ms
 * 0x2 3hz		333.333ms
 * 0x3 7.5hz	133.333ms
 * 0x4 15hz (default) 66.6667ms
 * 0x5 30hz		33.3333ms
 * 0x6 75hz		13.3333ms**/
#define M_SENSOR_DEFAULT_RATE 0x4

/**zxsu: refactor here to support more than one sensor **/
static sensors_data_t sensors;

/*@brief
 * Sensor control device context
 */
struct sensors_control_context_t {
	struct sensors_control_device_t device;
	int control_fd[SENSORS_FD_COUNT];
	uint32_t active_sensors; //TODO: 
};

/*@brief
 * Sendor data device context
 */
struct sensors_data_context_t {
	struct sensors_data_device_t device;
	int events_fds[SENSORS_FD_COUNT];
	sensors_data_t sensors[SENSORS_SUPPORT_COUNT];
	uint32_t pending_sensors;  //TODO: pending_sensors useless
};

/*
 * the following is the list of all supported sensors
 * zxsu: notice add magnitic field sensor here!
 */
static const struct sensor_t broncho_sensor_list[] = {
	//accelerometer
    {
      .name = "Lis35DE 3-axis Accelerometer",
      .vendor = "STMicroelectronics",
      .version = 1,
      .handle = ID_ACCELERATION,
      .type = SENSOR_TYPE_ACCELEROMETER,
      .maxRange = (GRAVITY_EARTH * 2.3f),
      .resolution = (GRAVITY_EARTH * 2.3f) / 128.0f,
      .power = 0.5f,
      .reserved = {},
    },
    //megnetic
	{
		.name = "HMC5883L magnetic field sensor",
		.vendor = "Honeywell",
		.version = 1,
		.handle = ID_MAGNETIC_FIELD,
		.type = SENSOR_TYPE_MAGNETIC_FIELD,
	//	.maxRange = (MAGNETIC_FIELD_EARTH_MAX * 10.0f),
		.maxRange = 800,
		.resolution = 0.5f,
		.power = 0.1f,
		.reserved = {},
	},
	//TODO: add orintation 
};

/** zxsu: notice return the number of sensors **/
static int  sensors_get_list(struct sensors_module_t *module,
                                 struct sensor_t const** list)
{
    *list = broncho_sensor_list;
    return (sizeof(broncho_sensor_list)/sizeof(struct sensor_t));
}

/** Close the sensors device */
static int
__common_close_sensors(struct hw_device_t *device) 
{
	struct sensors_data_context_t *dev;
	dev = (struct sensors_data_context_t *)device;

	if (dev) {
		int i = 0;
		for(i = 0; i < SENSORS_SUPPORT_COUNT; i++) {
			close(dev->events_fds[i]);
		}
		
		free(dev);
	}
	
	return 0;
}

int control_open_input(char* dev_name, int mode)
{
	int fd = -1;
	const char* dirname = "/dev/input";
	char devname[PATH_MAX];
	char* filename;
	DIR* dir;
	struct dirent* de;
	
	dir = opendir(dirname);
	if (dir == NULL) {
		return -1;
	}

	strcpy(devname, dirname);
	filename = devname + strlen(devname);
	*filename++ = '/';
	
	while((de = readdir(dir))) {
		if ((de->d_name[0] == '.' && de->d_name[1] == '\0') ||
			(de->d_name[0] == '.' && de->d_name[1] == '.'  &&
			 de->d_name[2] == '\0')) {
			continue;
		}
		strcpy(filename, de->d_name);
		fd = open(devname, mode);
		if(fd >= 0) {
			char name[80];
			if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
				name[0] = '\0';
			}

			if(!strcmp(name, dev_name)) {
				LOGD("Using %s (name = %s)", devname, name);
				break;
			}

			close(fd);
			fd = -1;
		}
	}

	closedir(dir);

	if (fd < 0) {
		LOGD("Couldn't find or open %s driver", dev_name);
	}

	return fd;
}

static native_handle_t* __control_open_data_source(struct sensors_control_device_t *device)
{
    struct sensors_control_context_t *dev;

    native_handle_t *sensor_handle;
    int fds[SENSORS_SUPPORT_COUNT];
    
    dev = (struct sensors_control_context_t*)device;

	int i = 0;
	for (i = 0; i < SENSORS_SUPPORT_COUNT; i++) {
		switch(i+1) {
			case ID_ACCELERATION:
				fds[i] = control_open_input(G_SENSOR_NAME, O_RDONLY);
				break;
			case ID_MAGNETIC_FIELD:
				fds[i] = control_open_input(M_SENSOR_NAME, O_RDONLY);
				break;
			default:
				break;
		}
	}

    sensor_handle = native_handle_create (SENSORS_FD_COUNT, 0);
    for (i = 0; i < SENSORS_FD_COUNT; i++) {
    	sensor_handle->data[i] = fds[i];
	}

    return sensor_handle;
}

/*
//TODO del this function
static int control_open_control_fd(struct sensors_control_context_t* dev, int handle)
{
	if (dev->control_fd[handle - 1] < 0) {
		switch(handle) {
			case ID_ACCELERATION:
				dev->control_fd[handle - 1] = open(G_SENSOR_DEVICE, O_WRONLY);
				break;
			case ID_MAGNETIC_FIELD:
				dev->control_fd[handle - 1] = open(M_SENSOR_DEVICE, O_WRONLY);
				break;
			default:
				break;
		}
	}

	return dev->control_fd[handle - 1];
}
*/

/**
static void control_enable_disable_sensors(uint32_t sensors, uint32_t mask)
{
	short enable;

	if (mask & SENSORS_ACCELERATION) {
		// control acceleration here!!
		enable = (sensors & SENSORS_ACCELERATION) ? 1 : 0;
		int control_fd = open(G_SENSOR_DEVICE, O_WRONLY);
		if (control_fd > 0)
        {
            if (enable)
            {
                char cmd[32];
                sprintf (cmd, G_SENSOR_CMD_START_TIMER, (uint)G_SENSOR_DEFAULT_TIMER);
                write(control_fd, cmd, sizeof (cmd));
                write(control_fd, G_SENSOR_CMD_POWER_ON, sizeof (G_SENSOR_CMD_POWER_ON));
            }
            else
            {
                write(control_fd, G_SENSOR_CMD_STOP_TIMER, sizeof (G_SENSOR_CMD_STOP_TIMER));
                write(control_fd, G_SENSOR_CMD_POWER_OFF, sizeof (G_SENSOR_CMD_POWER_OFF));
            }

            close(control_fd);
        }
	}

	if (mask & SENSORS_MAGNETIC_FIELD) {
		enable = (sensors & SENSORS_MAGNETIC_FIELD) ? 1 : 0;
		int control_fd = open(M_SENSOR_DEVICE, O_WRONLY);
		if (control_fd > 0)
		{
			if(enable)
			{
				char cmd[32];
				sprintf(cmd, M_SENSOR_CMD_START_RATE, (uint)M_SENSOR_DEFAULT_RATE);
				write(control_fd, cmd, sizeof(cmd));
			}
			else 
			{
				write(control_fd, M_SENSOR_CMD_STOP_RATE, sizeof(M_SENSOR_CMD_STOP_RATE));
			}

			close(control_fd);
		}
	}
}
**/

static void control_enable_disable_sensors(int handle, int enabled)
{
	int control_fd = -1;
	switch (handle) {
		case ID_ACCELERATION:
			// control acceleration here!!
			control_fd = open(G_SENSOR_DEVICE, O_WRONLY);
			if (control_fd > 0)
			{
				if (enabled)
				{
					char cmd[32];
					sprintf (cmd, G_SENSOR_CMD_START_TIMER, (uint)G_SENSOR_DEFAULT_TIMER);
					write(control_fd, cmd, sizeof (cmd));
					write(control_fd, G_SENSOR_CMD_POWER_ON, sizeof (G_SENSOR_CMD_POWER_ON));
				}
				else
				{
					write(control_fd, G_SENSOR_CMD_STOP_TIMER, sizeof (G_SENSOR_CMD_STOP_TIMER));
					write(control_fd, G_SENSOR_CMD_POWER_OFF, sizeof (G_SENSOR_CMD_POWER_OFF));
				}

				close(control_fd);
			}
			break;
		case ID_MAGNETIC_FIELD:
			// control magnetic field
			control_fd = open(M_SENSOR_DEVICE, O_WRONLY);
			if (control_fd > 0)
			{
				if(enabled)
				{
					char cmd[32];
					//sprintf(cmd, M_SENSOR_CMD_START_RATE, (uint)M_SENSOR_DEFAULT_RATE);
					sprintf(cmd, M_SENSOR_CMD_START);
					write(control_fd, cmd, sizeof(cmd));
				}
				else 
				{
					write(control_fd, M_SENSOR_CMD_STOP_RATE, sizeof(M_SENSOR_CMD_STOP_RATE));
				}

				close(control_fd);
			}
			break;
	}
}

/**
static int control_activate(struct sensors_control_device_t *device,
                            int handle, int enabled)
{
	struct sensors_control_context_t *dev;

	dev = (struct sensors_control_context_t *)device;
	LOGD("+%s: handle=%d enabled=%d", __FUNCTION__, handle, enabled);

	if((handle < (SENSORS_HANDLE_BASE+1)) ||
			(handle > SENSORS_HANDLE_BASE + SENSORS_SUPPORT_COUNT)) {
		return -1;
	}

	uint32_t mask = (1 << (handle-1));
	uint32_t sensors = enabled ? mask : 0;

	uint32_t active = dev->active_sensors;
	uint32_t new_sensors = (active & ~mask) | (sensors & mask);
	uint32_t changed = active ^ new_sensors;
	if (!changed) {
		return 0;
	}

	// get controll fd here!!
	int fd = control_open_control_fd(dev, handle);
	if (fd < 0) {
		return -1;
	}

	if (!active && new_sensors) {
		//if have none active sensors and have new_sensors, then force all sensors to updated
		changed = SUPPORTED_SENSORS;
	}

	control_enable_disable_sensors(fd, new_sensors, changed);
    return 0; 
} **/

static int __control_activate(struct sensors_control_device_t *device,
                            int handle, int enabled)
{
	struct sensors_control_context_t *dev;

	dev = (struct sensors_control_context_t *)device;
	LOGD("+%s: handle=%d enabled=%d", __FUNCTION__, handle, enabled);

	if((handle < (SENSORS_HANDLE_BASE+1)) ||
			(handle > SENSORS_HANDLE_BASE + SENSORS_SUPPORT_COUNT)) {
		return -1;
	}

	control_enable_disable_sensors(handle, enabled);
    return 0; 
}

static int __control_set_delay(struct sensors_control_device_t *dev, int32_t ms)
{
    LOGD ("%s %d\n", __func__, ms);
    int g_sensor_fd = open (G_SENSOR_DEVICE,  O_WRONLY);
    if (g_sensor_fd > 0)
    {
        char cmd[32];
        sprintf (cmd, G_SENSOR_CMD_START_TIMER, (uint)(ms < 20 ? 20 : ms));
        write (g_sensor_fd, cmd, sizeof (cmd));
        close (g_sensor_fd);
    }

	int m_sensor_fd = open (M_SENSOR_DEVICE, O_WRONLY);
	if (m_sensor_fd > 0) 
	{
		char cmd[32];
		sprintf(cmd, M_SENSOR_CMD_START_RATE, (uint)ms);
		write(m_sensor_fd, cmd, sizeof(cmd));
		close(m_sensor_fd);
	}

    return 0;
}

//TODO may be driver support.
static int __control_wake(struct sensors_control_device_t *dev)
{
    return 0;
}

static int __control_close(struct hw_device_t *device)
{
	struct sensors_control_context_t *dev;

	dev = (struct sensors_control_context_t *)device;
	int i = 0;
	if (dev) {
		for(i = 0; i < SENSORS_FD_COUNT; i++) {
			if (dev->control_fd[i] >= 0) {
				close(dev->control_fd[i]);
			}
		}
		free(dev);
	}

	return 0;
}

int __sensors_data_open(struct sensors_data_device_t *device, native_handle_t *sensors_handle)
{
	struct sensors_data_context_t *dev;
	dev = (struct sensors_data_context_t *)device;

	int i;
	memset(&dev->sensors, 0, sizeof(dev->sensors));

	for(i = 1; i <= SENSORS_SUPPORT_COUNT; i++) {
		dev->events_fds[i-1] = dup(sensors_handle->data[i-1]);
		switch (i) {
			case ID_ACCELERATION:
			case ID_MAGNETIC_FIELD:
				dev->sensors[i-1].vector.status = 0;
			default:
				break;
		}
	}

	dev->pending_sensors = 0;
	native_handle_close(sensors_handle);
	native_handle_delete(sensors_handle);
    //sensors.vector.status = SENSOR_STATUS_ACCURACY_HIGH;
    return 0;
}

int __sensors_data_close(struct sensors_data_device_t *device)
{
	struct sensors_data_context_t *dev;
	dev = (struct sensors_data_context_t *)device;
	
	int i = 0;
	for (i = 0; i < SENSORS_SUPPORT_COUNT; i++) {
		if (dev->events_fds[i] >= 0) {
			close(dev->events_fds[i]);
			dev->events_fds[i] = -1;
		}
	}

    return 0;
}

#define ACCEL_MOTION_SHAKE_RIGHT 0
#define ACCEL_MOTION_SHAKE_LEFT  1

int __sensors_data_poll(struct sensors_data_device_t *device, sensors_data_t* values)
{
	struct sensors_data_context_t *dev;
	dev = (struct sensors_data_context_t *)device;
	int nr;
	uint32_t g_sensor = 0;
	uint32_t m_sensor = 0;
	int i = 0;

	struct pollfd* mfds = (struct pollfd *)calloc(SENSORS_FD_COUNT, sizeof(struct pollfd));

	for(i = 0; i < SENSORS_FD_COUNT; i++) {
		mfds[i].fd = dev->events_fds[i];
		mfds[i].events = POLLIN;
	}

	while(1) {
		struct input_event event;
		nr = poll(mfds, SENSORS_FD_COUNT, 0);
		if (nr < 0) {
			continue;
		}

		for (i = 0; i < SENSORS_FD_COUNT; i++)
		{
			if (mfds[i].revents == POLLIN)
			{
				int ret = read(mfds[i].fd, &event, sizeof(event));
				if (ret < sizeof(event)) {
					break;
				}

				if (event.type == EV_ABS) {
					//handle input event 
					switch(event.code) {
						case ABS_X:
							if (i == (ID_ACCELERATION - 1)) {
								g_sensor |= AXIS_X;
								dev->sensors[i].acceleration.x = event.value * G_SENSOR_CONVERT;
							} else if (i == (ID_MAGNETIC_FIELD - 1)) {
								m_sensor |= AXIS_X;
								dev->sensors[i].magnetic.x = event.value * M_SENSOR_CONVERT;
							}
							break;
						case ABS_Y:
							if (i == (ID_ACCELERATION - 1)) {
								g_sensor |= AXIS_Y;
								dev->sensors[i].acceleration.y = event.value * G_SENSOR_CONVERT;
							}else if(i == (ID_MAGNETIC_FIELD - 1)) {
								m_sensor |= AXIS_Y;
								dev->sensors[i].magnetic.y = event.value * M_SENSOR_CONVERT;
							}
							break;
						case ABS_Z:
							if (i == (ID_ACCELERATION - 1)) {
								g_sensor |= AXIS_Z;
								dev->sensors[i].acceleration.z = event.value * G_SENSOR_CONVERT;
							}else if(i == (ID_MAGNETIC_FIELD - 1)) {
								m_sensor |= AXIS_Z;
								dev->sensors[i].magnetic.z = event.value * M_SENSOR_CONVERT;
							}
							break;
						default:
							break;
					}
				} else if (event.type == EV_SYN) {
					int64_t t = event.time.tv_sec * SEC_TO_NSEC + event.time.tv_usec *
						USEC_TO_NSEC;

					if (g_sensor & SENSORS_AXIS_ALL) {
						//g_sensor is ready
						g_sensor = 0;
						dev->sensors[i].sensor = ID_ACCELERATION;
						dev->sensors[i].time = t;
						*values = dev->sensors[i];
						return ID_ACCELERATION;
					} else if(m_sensor & SENSORS_AXIS_ALL) {
						//m_sensor is ready
						m_sensor = 0;
						dev->sensors[i].sensor = ID_MAGNETIC_FIELD;
						dev->sensors[i].time = t;
						*values = dev->sensors[i];
						return ID_MAGNETIC_FIELD;
					}
				}
			}
		}
	}
	
    return 0;
}

/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a sensors device using name */
/** zxsu:notice renaem SENSORS_HARDWARE_CONTROL and DATA, add MAGNITIC PART **/
static int open_sensors(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int status = -EINVAL;
    int i = 0;

    if (!strcmp(name, SENSORS_HARDWARE_CONTROL))
    {
        struct sensors_control_context_t *device_control;
        device_control = (struct sensors_control_context_t *)malloc(sizeof(*device_control));
        if (!device_control) {
        	return status;
		}

        memset(device_control, 0, sizeof(*device_control));
		//MAYBE: maybe SENSORS_FD_COUNT here
		for (i = 0; i < SENSORS_SUPPORT_COUNT; i++) {
			device_control->control_fd[i] = -1;
		}

        device_control->device.common.tag = HARDWARE_DEVICE_TAG;
        device_control->device.common.version = 0;
        device_control->device.common.module = (struct hw_module_t*)module;
        device_control->device.common.close = __control_close;
        device_control->device.open_data_source = __control_open_data_source;
        device_control->device.activate = __control_activate;
        device_control->device.set_delay = __control_set_delay;
        device_control->device.wake = __control_wake;
        *device = &device_control->device.common;
        status = 0;
    }
    else if (!strcmp(name, SENSORS_HARDWARE_DATA)) {
    	struct sensors_data_context_t *device_data;
        device_data = (struct sensors_data_context_t *)malloc(sizeof(*device_data));
        if (!device_data) {
        	return status;
		}

        memset(device_data, 0, sizeof(*device_data));
        for (i = 0; i < SENSORS_FD_COUNT; i++) {
        	device_data->events_fds[i] = -1;
		}

        device_data->device.common.tag = HARDWARE_DEVICE_TAG;
        device_data->device.common.version = 0;
        device_data->device.common.module = (struct hw_module_t*)module;
        device_data->device.common.close = __common_close_sensors;
        device_data->device.data_open = __sensors_data_open;
        device_data->device.data_close = __sensors_data_close;
        device_data->device.poll = __sensors_data_poll;
        *device = &device_data->device.common;
        status = 0;
    }
    return status;
}

static struct hw_module_methods_t sensors_module_methods = {
    .open =  open_sensors,
};

/*
 * The Sensors Hardware Module
 */
const struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "A1 Sensors Module",
        .author = "Broncho Team",
        .methods = &sensors_module_methods,
     },
     .get_sensors_list = sensors_get_list,
};

/*
 * handle_special_motion backup
 */
#define ACCEL_MOTION_SHAKE_RIGHT 0
#define ACCEL_MOTION_SHAKE_LEFT  1
static inline int get_uinput_fd ()
{
    int fd;
    struct uinput_dev dev;

    fd = open("/dev/uinput", O_RDWR);
    if (fd < 0)
    {
        return -1;
    }

    memset(&dev, 0, sizeof(dev));
    dev.id.bustype = 0x06;
    dev.id.vendor  = 0x0000;
    dev.id.product = 0x0000;
    dev.id.version = 0x0000;

    if (write(fd, &dev, sizeof(dev)) < 0) 
    {
        close (fd);
        return -1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_EVBIT, EV_REP);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    ioctl(fd, UI_SET_KEYBIT, KEY_LEFT);
    ioctl(fd, UI_SET_KEYBIT, KEY_RIGHT);

    if (ioctl(fd, UI_DEV_CREATE, NULL) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static inline void send_uinput_event (int fd, uint16_t type, uint16_t code, int32_t value)
{
    struct uinput_event event;

    memset(&event, 0, sizeof(event));
    event.type  = type;
    event.code  = code;
    event.value = value;

    write(fd, &event, sizeof(event));

    return;
}

static inline void send_motion_event (int event)
{
    static int uinput_fd = -1;

    if (uinput_fd < 0)
    {
        uinput_fd = get_uinput_fd ();
    }

    int key = event == ACCEL_MOTION_SHAKE_LEFT ? KEY_LEFT : KEY_RIGHT;

    send_uinput_event(uinput_fd, EV_KEY, key, 1);
    send_uinput_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
    send_uinput_event(uinput_fd, EV_KEY, key, 0);
    send_uinput_event(uinput_fd, EV_SYN, SYN_REPORT, 0);

    return;
}

static void handle_special_motion ()
{
    static int pre_Gx = 0;
    static int cur_Gx = 0;
    static time_t timestamp = 0;

    time_t tempstamp = time (NULL);
    if ((tempstamp - timestamp) < 1)
    {
        cur_Gx = sensors.acceleration.x * 1000;

        if ((pre_Gx > 150 && cur_Gx < -200) || (pre_Gx < -150 && cur_Gx > 200))
        {
            LOGE ("special event : %s \n", pre_Gx > 0 ? "Shake Right" : "Shake Left");
            send_motion_event (pre_Gx > 0 ? ACCEL_MOTION_SHAKE_RIGHT : ACCEL_MOTION_SHAKE_LEFT);

            pre_Gx = 0;
            cur_Gx = 0;
        }
    }
    else
    {
        pre_Gx = sensors.acceleration.x * 1000;
        cur_Gx = 0;
        timestamp = tempstamp;
    }

    return;
}
