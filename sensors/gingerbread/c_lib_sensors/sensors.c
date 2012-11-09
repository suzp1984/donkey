#include <hardware/sensors.h>
#include "common.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))


/*
 * The SENSORS Module
 **/

static const struct sensor_t sSensorList[] = {
	{ "Lis35DE 3-axis Accelerometer",
		"STMicroelectronics",
		1, SENSORS_HANDLE_BASE+ID_A,
		SENSOR_TYPE_ACCELEROMETER, 4.0f*9.81f, (4.0f*9.81f)/256.0f, 0.2f, 0, { } },
	{ "AK8975 3-axis Magnetic field sensor",
		"Asahi Kasei Microdevices",
		1,
		SENSORS_HANDLE_BASE + ID_M,
		SENSOR_TYPE_MAGNETIC_FIELD, 1228.8f,
		0.06f, 0.35f, 10000, { } },
	{ "Broncho Orientation sensor",
		"Broncho",
		1, SENSORS_HANDLE_BASE+ID_O,
		SENSOR_TYPE_ORIENTATION, 360.0f, 1.0f, 7.0f, 0, { } },
	{ "TMD 2771 Proximity sensor",
		"TMD 2771",
		1, SENSORS_HANDLE_BASE+ID_P,
		SENSOR_TYPE_PROXIMITY,
		1, 1,
		0.5f, 0, { } },
	{ "TAOS 258x ambient Light sensor",
		"TAOS Inc.",
		1, SENSORS_HANDLE_BASE+ID_L,
		SENSOR_TYPE_LIGHT, 10240.0f, 1.0f, 0.5f, 0, { } },
};

static int sensors_get_list(struct sensors_module_t* module, 
		struct sensor_t const** list)
{
	*list = sSensorList;

	return ARRAY_SIZE(sSensorList);
}

static int open_sensors(const struct hw_module_t* module, const char* name, 
		struct hw_device_t** device)
{
	int ret = -1;
	if (!strcmp(name, SENSORS_HARDWARE_POLL)) {
		SensorsPollContext* ctx = sensors_poll_context_create(module);
		*device = &ctx->device.common;

		ret = 0;
	}

	return ret;
}

static struct hw_module_methods_t sensors_module_methods = {
	.open = open_sensors
};

/*
 * The Sensors Hardware Module
 **/
const struct sensors_module_t HAL_MODULE_INFO_SYM = {
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.version_major = 1,
		.version_minor = 0,
		.id = SENSORS_HARDWARE_MODULE_ID,
		.name = "Broncho Sensors Module",
		.author = "Broncho Team",
		.methods = &sensors_module_methods,
	},
	.get_sensors_list = sensors_get_list,
};
