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

/* calibrate:
 * X_H = X*cos(poll) + Y*sin(pitch)*sin(poll) - Z*cos(pitch)*sin(poll)
 * Y_H = Y*cos(pitch) + Z * sin(pitch);
 *
 * azimath = arcTan(Y_H/X_H);
 *
 * X = Xsf * X + X_off
 * Y = Ysf * Y + Y_off
 * Z = Z + Z_off
 *
 * Xsf = max(1, (Y_max - Y_min)/(X_max - X_min))
 * Ysf = max(1, (X_max - X_min)/(Y_max - Y_min))
 * Zsf = 1
 *
 * X_off = [(X_max - X_min)/2 - X_max] * Xsf
 * Y_off = [(Y_max - Y_min)/2 - Y_max] * Ysf
 * Z_off = [(Z_max - Z_min)/2 - Z_max] * Zsf
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
#define SENSORS_ORIENTATION 	(1 << (ID_ORIENTATION - 1))
#define INPUT_DIR               "/dev/input"
#define SUPPORTED_SENSORS       ((1 << SENSORS_SUPPORT_COUNT) - 1)

#define G_M_SENSOR_ALL (SENSORS_ACCELERATION | SENSORS_MAGNETIC_FIELD)
//#define EVENT_MASK_ACCEL_ALL    ( (1 << ABS_X) | (1 << ABS_Y) | (1 << ABS_Z))
//#define DEFAULT_THRESHOLD 100

#define AXIS_X (1 << ABS_X)
#define AXIS_Y (1 << ABS_Y)
#define AXIS_Z (1 << ABS_Z)
#define SENSORS_AXIS_ALL (AXIS_X | AXIS_Y | \
                                  AXIS_Z)
//#define AXIS_G_SENSOR (1 << ID
#define SEC_TO_NSEC 1000000000LL
#define USEC_TO_NSEC 1000
#define PI 3.14

/** zxsu: add G_SENSOR_NAME and M_SENSOR_NAME here **/
#define G_SENSOR_NAME "g_sensor"
#define M_SENSOR_NAME "m_sensor"

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

/**template: add m_sensor **/
#define Y_off -51.7
#define X_off -45.28
#define Z_off 25.6
#define Ysf 1
#define Xsf 1.6
/** supported frequency of m_sensor: 
 * 0x0 0.75hz  1333.33ms
 * 0x1 1.5hz	666.667ms
 * 0x2 3hz		333.333ms
 * 0x3 7.5hz	133.333ms
 * 0x4 15hz (default) 66.6667ms
 * 0x5 30hz		33.3333ms
 * 0x6 75hz		13.3333ms**/
//#define M_SENSOR_DEFAULT_RATE 0x4


/*@brief
 * Sensor control device context
 */
struct sensors_control_context_t {
	struct sensors_control_device_t device;
	int control_fd[SENSORS_FD_COUNT]; //XXX:
	uint32_t active_sensors; //Enhancement
	void (*control_sensors)(struct sensors_control_context_t* dev, int handle, int enbale);
};

/*@brief
 * Sendor data device context
 */
struct sensors_data_context_t {
	struct sensors_data_device_t device;
	//int events_fds[SENSORS_FD_COUNT];  //TODO:XXX pollfd instead of events fd
	struct pollfd* mfds; 
	sensors_data_t* sensors;
	sensors_data_t pending_g_sensor;
	sensors_data_t pending_m_sensor;
	uint32_t pending_sensors;  //TODO: pending_sensors useless
	uint32_t g_sensor_axis;
	uint32_t m_sensor_axis;
	int (*m_sensor_calibrate)(struct sensors_data_context_t* dev, sensors_data_t* m_data);
	sensors_data_t (*gen_orientation)(sensors_data_t m_sensor, sensors_data_t g_sensor);
	//calebrate var
	/*
	float Xsf;
	float Xoff;
	float Ysf;
	float Yoff;
	float Ymax;
	float Xmax;
	float Ymin;
	float Xmin;
	float Zmax;
	float Zmin;
	*/
};

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
		/*
		 * close mfds in data_close
		int i = 0;
		for(i = 0; i < SENSORS_FD_COUNT; i++) {
			if(dev->mfds[i].fd >= 0) {
				close(dev->mfds[i].fd);
				dev->mfds[i].fd = -1;	
			}
		}*/
		
		free(dev);
	}
	LOGE("__common_close_sensors:.....");
	
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
				LOGE("Using %s (name = %s)", devname, name);
				break;
			}

			close(fd);
			fd = -1;
		}
	}

	closedir(dir);

	if (fd < 0) {
		LOGE("Couldn't find or open %s driver", dev_name);
	}

	return fd;
}

static native_handle_t* __control_open_data_source(struct sensors_control_device_t *device)
{
    struct sensors_control_context_t *dev;

    native_handle_t *sensor_handle;
    int fds[SENSORS_FD_COUNT];
    
    dev = (struct sensors_control_context_t*)device;

	int i = 0;
	int fd_num = 0;
	for (i = 0; i < SENSORS_FD_COUNT; i++) {
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

	//XXX:TODO: handle must not be -1;
    sensor_handle = native_handle_create (SENSORS_FD_COUNT, 0);
    for (i = 0; i < SENSORS_FD_COUNT; i++) {
    	sensor_handle->data[i] = fds[i];
	}

	LOGE("__control_open_data_source:......");
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

static void control_g_sensors(struct sensors_control_context_t* dev, int enabled)
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
			dev->active_sensors |= SENSORS_ACCELERATION;
		}
		else
		{
			write(control_fd, G_SENSOR_CMD_STOP_TIMER, sizeof (G_SENSOR_CMD_STOP_TIMER));
			write(control_fd, G_SENSOR_CMD_POWER_OFF, sizeof (G_SENSOR_CMD_POWER_OFF));
			dev->active_sensors &= ~SENSORS_ACCELERATION;
		}

		close(control_fd);
	}
}

static void control_m_sensors(struct sensors_control_context_t* dev, int enabled)
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
			dev->active_sensors |= SENSORS_MAGNETIC_FIELD;
		}
		else 
		{
			write(control_fd, M_SENSOR_CMD_STOP_RATE, sizeof(M_SENSOR_CMD_STOP_RATE));
			dev->active_sensors &= ~SENSORS_MAGNETIC_FIELD;
		}

		close(control_fd);
	}
}

//XXX: open both g_sensor and m_sensor all, if handle = ID_ORIENTATION:
//TODO: enable and disable sensors
static void control_enable_disable_sensors(struct sensors_control_context_t* dev, int handle, int enabled)
{
	//XXX: BUGS maybe
	switch(handle)
	{
		case ID_ACCELERATION:
			if((dev->active_sensors & SENSORS_ACCELERATION) && (!enabled)) {
				//acceleration is enabled; and want to disable acceleration
				control_g_sensors(dev, enabled);
			} else if(!(dev->active_sensors & SENSORS_ACCELERATION) && enabled) {
				//acceleration is disabled, and want to enable acceleration
				control_g_sensors(dev, enabled);
			}
			break;
		case ID_MAGNETIC_FIELD:
			if((dev->active_sensors & SENSORS_MAGNETIC_FIELD) && (!enabled)) {
				//magnetic is enabled; and want to disable it
				control_m_sensors(dev, enabled);
			} else if(!(dev->active_sensors & SENSORS_MAGNETIC_FIELD) && enabled) {
				//magneticis disabled, whatever
				control_m_sensors(dev, enabled);
			}
			break;
		case ID_ORIENTATION:
			if(enabled && !(dev->active_sensors & SENSORS_ACCELERATION)) {
				/*want to enable orientation, and acceleration is disabled, 
				 * enable acceleration
				 */
				control_g_sensors(dev, enabled);
			}

			if(enabled && !(dev->active_sensors & SENSORS_MAGNETIC_FIELD)) {
				/*want to enable orientation, and magnetic is disabled, 
				 * enable magnetic 
				 */
				control_m_sensors(dev, enabled);
			}

			/* do not disable whatever
			if(!enabled && (dev->active_sensors & SENSORS_ACCELERATION)) {
				control_g_sensors(dev, enabled);
			}*/

			if(!enabled && (dev->active_sensors & SENSORS_MAGNETIC_FIELD)) {
				control_m_sensors(dev, enabled);
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
	struct sensors_control_context_t* dev;

	dev = (struct sensors_control_context_t*)device;
	LOGE("+%s: handle=%d enabled=%d", __FUNCTION__, handle, enabled);

	if((handle < (SENSORS_HANDLE_BASE+1)) ||
			(handle > SENSORS_HANDLE_BASE + SENSORS_SUPPORT_COUNT)) {
		LOGW("+%s: handle=%d  enable this handle failed!!", __FUNCTION__, handle);
		return -1;
	}

	dev->control_sensors(dev, handle, enabled);

	return 0; 
}

//XXX: 20ms is the fastest
static int __control_set_delay(struct sensors_control_device_t *device, int32_t ms)
{
    struct sensors_control_context_t* dev;
    dev = (struct sensors_control_context_t*)device;

	int32_t msecond = ms;
    if(ms < 20) {
    	msecond = 20;
	}

	if(dev->active_sensors & SENSORS_ACCELERATION) {
    	LOGE ("g_sensor: %s %d\n", __func__, msecond);
		int g_sensor_fd = open (G_SENSOR_DEVICE,  O_WRONLY);
		if (g_sensor_fd > 0)
		{
			char cmd[32];
			sprintf (cmd, G_SENSOR_CMD_START_TIMER, (uint)(msecond < 20 ? 20 : msecond));
			write (g_sensor_fd, cmd, sizeof (cmd));
			close (g_sensor_fd);
		}
	}

	if(dev->active_sensors & SENSORS_MAGNETIC_FIELD) {
    	LOGE ("m_sensor: %s %d\n", __func__, msecond);
		int m_sensor_fd = open (M_SENSOR_DEVICE, O_WRONLY);
		if (m_sensor_fd > 0) 
		{
			char cmd[32];
			sprintf(cmd, M_SENSOR_CMD_START_RATE, (uint)msecond);
			write(m_sensor_fd, cmd, sizeof(cmd));
			close(m_sensor_fd);
		}
	}

    return 0;
}

//TODO may be driver support.
static int __control_wake(struct sensors_control_device_t *dev)
{
	LOGE("__control_wake:......");
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
	LOGE("__control_close:....");

	return 0;
}

int __sensors_data_open(struct sensors_data_device_t *device, native_handle_t *sensors_handle)
{
	struct sensors_data_context_t *dev;
	dev = (struct sensors_data_context_t *)device;

	int i;
	int res;
	dev->mfds = (struct pollfd *)calloc((SENSORS_FD_COUNT), sizeof(struct pollfd));
	dev->sensors = (sensors_data_t*)calloc(SENSORS_SUPPORT_COUNT, sizeof(sensors_data_t));

	//TODO: must be the sensors_handle num
	for(i = 0; i < SENSORS_FD_COUNT; i++) {
		dev->mfds[i].fd = dup(sensors_handle->data[i]);
		dev->mfds[i].events = POLLIN;
		switch (i+1) {
			case ID_ACCELERATION:
			case ID_MAGNETIC_FIELD:
				dev->sensors[i].vector.status = 0;
			default:
				break;
		}
	}
	
	/*
	for(i = 0; i < SENSORS_FD_COUNT; i++) {
		dev->mfds[i].fd = dev->events_fds[i];
		dev->mfds[i].events = POLLIN;
	}*/

	dev->pending_sensors = 0;
	native_handle_close(sensors_handle);
	native_handle_delete(sensors_handle);
    //sensors.vector.status = SENSOR_STATUS_ACCURACY_HIGH;
    LOGE("__sensors_data_open:.......");
    return 0;
}

int __sensors_data_close(struct sensors_data_device_t *device)
{
	struct sensors_data_context_t *dev;
	dev = (struct sensors_data_context_t *)device;
	
	int i = 0;
	for (i = 0; i < SENSORS_FD_COUNT; i++) {
			close(dev->mfds[i].fd);
			dev->mfds[i].fd = -1;
	}

	free(dev->mfds);
	free(dev->sensors);
	LOGE("__sensors_data_close:.....");
    return 0;
}

#define ACCEL_MOTION_SHAKE_RIGHT 0
#define ACCEL_MOTION_SHAKE_LEFT  1

static sensors_data_t gen_orientation_v1(sensors_data_t m_sensor_data, sensors_data_t g_sensor_data)
{
	sensors_data_t o_sensor;
	float azimuth;
	float pitch;
	float roll;

	/**azimuth **/
	azimuth = atan2f(m_sensor_data.magnetic.x, m_sensor_data.magnetic.y);
	//radians --> angles
	azimuth = azimuth * 180 / PI;
	if(azimuth < 0) {
		azimuth = 360 + azimuth;
	}

	azimuth = 360-azimuth;

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
	/*
	if(m_sensor_data.magnetic.y < 0 && m_sensor_data.magnetic.x < 0) {
		azimuth = 360 - azimuth;
	} else if(m_sensor_data.magnetic.y > 0 && m_sensor_data.magnetic.x < 0) {
		azimuth = 360 - azimuth;
	}*/

	/** pitch **/
	//float z1 = - sqrtf (g_sensor_data.acceleration.x * g_sensor_data.acceleration.x + g_sensor_data.acceleration.z * g_sensor_data.acceleration.z);
	//pitch = atan2f (sensors.acceleration.y, z1)*180.0/3.14159;
	pitch = -atan2f (g_sensor_data.acceleration.y, g_sensor_data.acceleration.z)*180.0/PI;

	/** roll **/
	float z2 = - sqrtf (g_sensor_data.acceleration.y * g_sensor_data.acceleration.y + g_sensor_data.acceleration.z * g_sensor_data.acceleration.z);
	roll = -atan2f (g_sensor_data.acceleration.x, z2)*180.0/PI;

	if(roll < -90)
		roll += 180;
	if(roll > 90)
		roll -= 180;

	o_sensor.orientation.azimuth = azimuth;
	o_sensor.orientation.pitch = pitch;
	o_sensor.orientation.roll = roll;

	return o_sensor;
}

int __sensors_data_poll(struct sensors_data_device_t* device, sensors_data_t* values)
{
	struct sensors_data_context_t *dev;
	dev = (struct sensors_data_context_t *)device;
	int nr;
	int i = 0;
	int res = -1;

	while(1) {
		struct input_event event;
		nr = poll(dev->mfds, SENSORS_FD_COUNT, -1);
		if (nr < 0) {
			continue;
		}

		for(i = 0; i < SENSORS_FD_COUNT; i++)
		{
			if(dev->mfds[i].revents == POLLIN)
			{
				int ret = read(dev->mfds[i].fd, &event, sizeof(event));
				if(ret < sizeof(event)) {
					break;
				}

				int64_t t = event.time.tv_sec * SEC_TO_NSEC + event.time.tv_usec *
					USEC_TO_NSEC;

				if(event.type == EV_ABS) {
					
					//handle input event 
					switch(event.code) {
						case ABS_X:
							if (i == (ID_ACCELERATION-1)) {
								//g_sensor |= AXIS_X;
								dev->g_sensor_axis |= AXIS_X;
								dev->sensors[i].acceleration.x = event.value * G_SENSOR_CONVERT;
							} else if (i == (ID_MAGNETIC_FIELD-1)) {
								//m_sensor |= AXIS_X;
								dev->m_sensor_axis |= AXIS_X;
								dev->sensors[i].magnetic.x = event.value * M_SENSOR_CONVERT;
							}
							break;
						case ABS_Y:
							if (i == (ID_ACCELERATION-1)) {
								//g_sensor |= AXIS_Y;
								dev->g_sensor_axis |= AXIS_Y;
								dev->sensors[i].acceleration.y = event.value * G_SENSOR_CONVERT;
							}else if(i == (ID_MAGNETIC_FIELD-1)) {
								//m_sensor |= AXIS_Y;
								dev->m_sensor_axis |= AXIS_Y;
								dev->sensors[i].magnetic.y = event.value * M_SENSOR_CONVERT;
							}
							break;
						case ABS_Z:
							if (i == (ID_ACCELERATION-1)) {
								//g_sensor |= AXIS_Z;
								dev->g_sensor_axis |= AXIS_Z;
								dev->sensors[i].acceleration.z = event.value * G_SENSOR_CONVERT;
							}else if(i == (ID_MAGNETIC_FIELD-1)) {
								//m_sensor |= AXIS_Z;
								dev->m_sensor_axis |= AXIS_Z;
								dev->sensors[i].magnetic.z = event.value * M_SENSOR_CONVERT;
							}
							break;
						default:
							break;
					}
					//TODO:XXX
					// Orintation cal
					if(dev->pending_sensors & G_M_SENSOR_ALL) {
						dev->pending_sensors = 0;
						//get orientation data
						dev->sensors[ID_ORIENTATION-1] = dev->gen_orientation(dev->pending_m_sensor, dev->pending_g_sensor);
						LOGE("After gen_orientation data .....!!!!.........");
						dev->sensors[ID_ORIENTATION-1].sensor = ID_ORIENTATION;
						dev->sensors[ID_ORIENTATION-1].time = t;
						*values = dev->sensors[ID_ORIENTATION-1];

						return ID_ORIENTATION;
					}
				} else if (event.type == EV_SYN) {

					if (dev->g_sensor_axis & SENSORS_AXIS_ALL) {
						//g_sensor is ready
						dev->g_sensor_axis = 0;
						dev->sensors[i].sensor = ID_ACCELERATION;
						dev->sensors[i].time = t;

						//pending g_sensor ::XXX:: bugs maybe
						dev->pending_g_sensor = dev->sensors[i];
						dev->pending_sensors |= SENSORS_ACCELERATION;
						*values = dev->sensors[i];
						LOGE("g_sensor value return!!");
						return ID_ACCELERATION;
					} else if(dev->m_sensor_axis & SENSORS_AXIS_ALL) {
						//m_sensor is ready
						dev->m_sensor_axis = 0;
						dev->sensors[i].sensor = ID_MAGNETIC_FIELD;
						dev->sensors[i].time = t;
						/*magnetic field calibrate */
						dev->m_sensor_calibrate(dev, &dev->sensors[i]);
						//pending m_sensor
						dev->pending_m_sensor = dev->sensors[i];
						dev->pending_sensors |= SENSORS_MAGNETIC_FIELD;
						*values = dev->sensors[i];
						LOGE("m_sensor value return!!");
						return ID_MAGNETIC_FIELD;
					} 			
				}
			}
		}
	}
	
    return 0;
}


static int m_sensor_calibrate_v1(struct sensors_data_context_t* dev, sensors_data_t* m_data)
{
	m_data->magnetic.x = m_data->magnetic.x * Xsf + X_off;
	m_data->magnetic.y = m_data->magnetic.y * Ysf + Y_off;
	m_data->magnetic.z = m_data->magnetic.z + Z_off;

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
		for (i = 0; i < SENSORS_FD_COUNT; i++) {
			device_control->control_fd[i] = -1;
		}

		//set control hook
		device_control->control_sensors = control_enable_disable_sensors;

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

		//set calibrate fun for m_sensor
		device_data->m_sensor_calibrate = m_sensor_calibrate_v1;
		//set generate orientation fun
		device_data->gen_orientation = gen_orientation_v1;
        
        /*
        for (i = 0; i < SENSORS_FD_COUNT; i++) {
        	device_data->mdfs[i].fd = -1;
		}*/

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
