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

#include <sys/inotify.h>
#include <math.h>
#include <sys/time.h>

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
#define SENSORS_SUPPORT_COUNT 3
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
	int events_fds[SENSORS_FD_COUNT];  //TODO:XXX pollfd instead of events fd
	sensors_data_t sensors[SENSORS_SUPPORT_COUNT];
	sensors_data_t pending_g_sensor;
	sensors_data_t pending_m_sensor;
	uint32_t pending_sensors;  //TODO: pending_sensors useless
};

/*
 * global mark var XXX:
 * XXX: refactor: rename to g_sensor_bit or remove the global vars
 */
uint32_t g_sensor = 0;
uint32_t m_sensor = 0;
uint32_t o_sensor = 0;
int pending_m_sensor = 0;
int pending_g_sensor = 0;

struct pollfd* mfds; 
UInput* udev;
/*
 * the following is the list of all supported sensors
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
	//XXX: maxRange (2) orintation 
	{
		.name = "Broncho compass Orientation sensor",
		.vendor = "Broncho",
		.version = 1,
		.handle = ID_ORIENTATION,
		.type = SENSOR_TYPE_ORIENTATION,
		.maxRange = 360,
		.resolution = 1.0f,
		.power = 0.0f,
		.reserved = {},
	},
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
		for(i = 0; i < SENSORS_FD_COUNT; i++) {
			if(dev->events_fds[i] > 0) {
				close(dev->events_fds[i]);
			}
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
			case ID_ORIENTATION:
				fds[i] = -1; //XXX:
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

static void control_g_sensors(int enabled)
{
	int control_fd = -1;

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
}

static void control_m_sensors(int enabled)
{
	int control_fd = -1;

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
}

//XXX: open both g_sensor and m_sensor all, if handle = ID_ORIENTATION:
//TODO: enable and disable sensors
static void control_enable_disable_sensors(int handle, int enabled)
{
	switch (handle) {
		case ID_ACCELERATION:
			// control acceleration here!!
			control_g_sensors(enabled);
			break;
		case ID_MAGNETIC_FIELD:
			// control magnetic field
			control_m_sensors(enabled);
			break;
		case ID_ORIENTATION:
			if(enabled)
			{
				//open g_sensor:
				control_g_sensors(enabled);
				//open m_sensor:
				control_m_sensors(enabled);
				LOGW("enabled orientation sensor!!");
			}
			break;
		default:
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
		LOGW("+%s: handle=%d  enable this handle failed!!", __FUNCTION__, handle);
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
	int res;
	mfds = (struct pollfd *)calloc((SENSORS_SUPPORT_COUNT + 1), sizeof(struct pollfd));
	udev = uinput_create(NULL, 600, 800);
	memset(&dev->sensors, 0, sizeof(dev->sensors));

	for(i = 1; i <= SENSORS_FD_COUNT; i++) {
		dev->events_fds[i-1] = dup(sensors_handle->data[i-1]);
		switch (i) {
			case ID_ACCELERATION:
			case ID_MAGNETIC_FIELD:
				dev->sensors[i-1].vector.status = 0;
			default:
				break;
		}
	}

	//add inotify
	mfds[0].events = POLLIN;
	mfds[0].fd = inotify_init();
	LOGW("____________inotify fd is: %d", mfds[0].fd);

	res = inotify_add_watch(mfds[0].fd, INPUT_DIR, IN_DELETE | IN_CREATE);
	if(res < 0) {
		LOGE("could not add watch for %s, %s\n", INPUT_DIR, strerror(errno));
	}
	for(i = 0; i < SENSORS_FD_COUNT; i++) {
		mfds[i+1].fd = dev->events_fds[i];
		mfds[i+1].events = POLLIN;
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
	for (i = 0; i < SENSORS_FD_COUNT; i++) {
			close(dev->events_fds[i]);
			dev->events_fds[i] = -1;
	}
	uinput_destroy(udev);

    return 0;
}

#define ACCEL_MOTION_SHAKE_RIGHT 0
#define ACCEL_MOTION_SHAKE_LEFT  1

static sensors_data_t gen_orientation(sensors_data_t m_sensor_data, sensors_data_t g_sensor_data)
{
	sensors_data_t o_sensor;
	float azimuth;
	float pitch;
	float roll;

	/**azimuth **/
	azimuth = atan2f(m_sensor_data.magnetic.x, m_sensor_data.magnetic.y);
	//radians --> angles
	azimuth = fabsf(azimuth) * 180 / M_PI;
	//azimuth [0, 360)
	//if(m_sensor_data.magnetic.y < 0 && m_sensor_data.magnetic.x > 0) {
	//} else if(m_sensor_data.magnetic.y < 0 && m_sensor_data.magnetic.x < 0) {
	//the atan2(-1, 1) is: 
	//-0.785398
	//the atan2(1, -1) is: 
	//2.356194
	//the atan2(-1, -1) is: 
	//-2.356194
	//
	if(m_sensor_data.magnetic.y < 0 && m_sensor_data.magnetic.x < 0) {
		azimuth = 360 - azimuth;
	} else if(m_sensor_data.magnetic.y > 0 && m_sensor_data.magnetic.x < 0) {
		azimuth = 360 - azimuth;
	}

	/** pitch **/
	float z1 = - sqrtf (g_sensor_data.acceleration.x * g_sensor_data.acceleration.x + g_sensor_data.acceleration.z * g_sensor_data.acceleration.z);
	//pitch = atan2f (sensors.acceleration.y, z1)*180.0/3.14159;
	pitch = atan2f (g_sensor_data.acceleration.y, g_sensor_data.acceleration.z)*180.0/M_PI;

	/** roll **/
	float z2 = - sqrtf (g_sensor_data.acceleration.y * g_sensor_data.acceleration.y + g_sensor_data.acceleration.z * g_sensor_data.acceleration.z);
	roll = atan2f (g_sensor_data.acceleration.x, z2)*180.0/M_PI;


	o_sensor.orientation.azimuth = azimuth;
	o_sensor.orientation.pitch = pitch;
	o_sensor.orientation.roll = roll;

	return o_sensor;
}

static int read_notify(int fd)
{
	struct inotify_event* event;
	char event_buf[512];
	char devname[PATH_MAX];
	char* filename;
	const char* dev_dir = "/dev/input";
	int event_pos = 0;
	int event_size;
	int dev_fd;
	int res;

	res = read(fd, event_buf, sizeof(event_buf));
	if(res < sizeof(*event)) {
		if(errno == EINTR) {
			return 0;
		}
		LOGW("Conld not get event, %s \n", strerror(errno));
		return 1;
	}

	strcpy(devname, dev_dir);
	filename = devname + strlen(devname);
	*filename++ = '/';

	while(res >= (int)sizeof(*event)) {
		event = (struct inotify_event*)(event_buf + event_pos);

		if(event->len) {
			strcpy(filename, event->name);
			if(event->mask & IN_CREATE) {
				//open_device
				dev_fd = open(filename, O_RDWR);
				return dev_fd;
			} else { 
				//close_device
				return -1;
			}
		}
		event_size = sizeof(*event) + event->len;
		res -= event_size;
		event_pos += event_size;
	}

	return -1;
}

int __sensors_data_poll(struct sensors_data_device_t *device, sensors_data_t* values)
{
	struct sensors_data_context_t *dev;
	dev = (struct sensors_data_context_t *)device;
	int nr;
	int i = 0;
	int res = -1;

	//TODO: pollfd to global var
	//struct pollfd* mfds = (struct pollfd *)calloc((SENSORS_FD_COUNT + 1), sizeof(struct pollfd));

	//create uinput
	//UInput* udev = uinput_create(NULL, 800, 600);
	//SYN event
	struct input_event o_sensor_syn;
	o_sensor_syn.type = 0;
	o_sensor_syn.code = 0;
	o_sensor_syn.value = 0;
	//TODO: inotify
	/*
	mfds[0].events = POLLIN;
	mfds[0].fd = inotify_init();
	LOGW("____________inotify fd is: %d", mfds[0].fd);
	
	res = inotify_add_watch(mfds[0].fd, INPUT_DIR, IN_DELETE | IN_CREATE);
	if(res < 0) {
		LOGE("could not add watch for %s, %s\n", INPUT_DIR, strerror(errno));
	}

	for(i = 0; i < SENSORS_FD_COUNT; i++) {
		mfds[i+1].fd = dev->events_fds[i];
		mfds[i+1].events = POLLIN;
	}
	*/

	while(1) {
		struct input_event event;
		//TODO:
		nr = poll(mfds, (SENSORS_SUPPORT_COUNT + 1), 0);
		if (nr < 0) {
			continue;
		}

		//mfds[0] was used for inotify. XXX: maybe ID_ORIENTATION 
		for(i = 1; i <= SENSORS_SUPPORT_COUNT; i++)
		{
			if(mfds[i].revents == POLLIN)
			{
				int ret = read(mfds[i].fd, &event, sizeof(event));
				if(ret < sizeof(event)) {
					break;
				}
				int64_t t = event.time.tv_sec * SEC_TO_NSEC + event.time.tv_usec *
					USEC_TO_NSEC;

				if(event.type == EV_ABS) {
					
					//handle input event 
					switch(event.code) {
						case ABS_X:
							if (i == (ID_ACCELERATION)) {
								g_sensor |= AXIS_X;
								dev->sensors[i-1].acceleration.x = event.value * G_SENSOR_CONVERT;
							} else if (i == (ID_MAGNETIC_FIELD)) {
								m_sensor |= AXIS_X;
								dev->sensors[i-1].magnetic.x = event.value * M_SENSOR_CONVERT;
							} else if (i == ID_ORIENTATION) {
								o_sensor |= AXIS_X;
								dev->sensors[i-1].orientation.x = event.value;
							}
							break;
						case ABS_Y:
							if (i == (ID_ACCELERATION)) {
								g_sensor |= AXIS_Y;
								dev->sensors[i-1].acceleration.y = event.value * G_SENSOR_CONVERT;
							}else if(i == (ID_MAGNETIC_FIELD)) {
								m_sensor |= AXIS_Y;
								dev->sensors[i-1].magnetic.y = event.value * M_SENSOR_CONVERT;
							}else if(i == ID_ORIENTATION) {
								o_sensor |= AXIS_Y;
								dev->sensors[i-1].orientation.y = event.value;
							}
							break;
						case ABS_Z:
							if (i == (ID_ACCELERATION)) {
								g_sensor |= AXIS_Z;
								dev->sensors[i-1].acceleration.z = event.value * G_SENSOR_CONVERT;
							}else if(i == (ID_MAGNETIC_FIELD)) {
								m_sensor |= AXIS_Z;
								dev->sensors[i-1].magnetic.z = event.value * M_SENSOR_CONVERT;
							}else if(i == ID_ORIENTATION) {
								o_sensor |= AXIS_Z;
								dev->sensors[i-1].orientation.z = event.value;
							}
							break;
						default:
							break;
					}
					// Orintation cal
					if(pending_g_sensor && pending_m_sensor) {
						LOGE("before gen_orientation data ......!!!!.......");
						pending_g_sensor = 0;
						pending_m_sensor = 0;
						//get orientation data
						dev->sensors[ID_ORIENTATION-1] = gen_orientation(dev->pending_m_sensor, dev->pending_g_sensor);
						LOGE("After gen_orientation data .....!!!!.........");
						//TODO: send SYN and set marks
						dev->sensors[ID_ORIENTATION-1].sensor = ID_ORIENTATION;
						dev->sensors[ID_ORIENTATION-1].time = t;
						*values = dev->sensors[ID_ORIENTATION-1];

						return ID_ORIENTATION;
					}

				} else if (event.type == EV_SYN) {
				//	int64_t t = event.time.tv_sec * SEC_TO_NSEC + event.time.tv_usec *
				//		USEC_TO_NSEC;
					//TODO: gen orientiaton data
					/*
					if(pending_g_sensor && pending_m_sensor) {
						LOGE("before gen_orientation data ......!!!!.......");
						pending_g_sensor = 0;
						pending_m_sensor = 0;
						//get orientation data
						dev->sensors[ID_ORIENTATION-1] = gen_orientation(dev->pending_m_sensor, dev->pending_g_sensor);
						LOGE("After gen_orientation data .....!!!!.........");
						//TODO: send SYN and set marks
						o_sensor |= AXIS_X;
						o_sensor |= AXIS_Y;
						o_sensor |= AXIS_Z;

						LOGE("before gettimeofday ......!!!!.......");
						gettimeofday(&o_sensor_syn.time, NULL);
						LOGE("After gettimeofday ......!!!!.......");
						if(udev->fd < 0)
						{ 
							LOGE("udev fd < 0 ---------------!!!!!!!!!");
						}

						write(udev->fd, &o_sensor_syn, sizeof(struct input_event));
						LOGE("After Write to /dev/uinput......!!!!.......");
					}
					*/

					if (g_sensor & SENSORS_AXIS_ALL) {
						//g_sensor is ready
						g_sensor = 0;
						dev->sensors[i-1].sensor = ID_ACCELERATION;
						dev->sensors[i-1].time = t;
						//pending g_sensor
						dev->pending_g_sensor = dev->sensors[i-1];
						pending_g_sensor = 1;
						*values = dev->sensors[i-1];
						return ID_ACCELERATION;
					} else if(m_sensor & SENSORS_AXIS_ALL) {
						//m_sensor is ready
						m_sensor = 0;
						dev->sensors[i-1].sensor = ID_MAGNETIC_FIELD;
						dev->sensors[i-1].time = t;
						//pending m_sensor
						dev->pending_m_sensor = dev->sensors[i-1];
						pending_m_sensor = 1;
						*values = dev->sensors[i-1];
						return ID_MAGNETIC_FIELD;
					} else if(o_sensor && SENSORS_AXIS_ALL) {
						//get o_sensor
						o_sensor = 0;
						dev->sensors[i-1].sensor = ID_ORIENTATION;
						dev->sensors[i-1].time = t;
						*values = dev->sensors[i-1];
						return ID_ORIENTATION;
					}
				}
			}
		}

		//TODO: inotify here!!!
		if(mfds[0].revents & POLLIN) {
			mfds[ID_ORIENTATION-1].fd = read_notify(mfds[0].fd);
			mfds[ID_ORIENTATION-1].revents = POLLIN;
		}
	}
	
    return 0;
}





/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a sensors device using name */
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
 *
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
*/
