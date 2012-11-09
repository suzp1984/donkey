#ifndef SENSORS_MANAGER_H
#define SENSORS_MANAGER_H

#include <hardware/sensors.h>
#include "common.h"

enum {
	accelerometer = 0,
	akm			  = 1,
	light 	      = 2,
	proximity     = 3,
	numSensorDrivers,
	numFds
};

struct _SensorsManager;
typedef struct _SensorsManager SensorsManager;

SensorsManager* sensors_manager_create();

int sensors_manager_activate(SensorsManager* thiz, int handle, int enabled);
int sensors_manager_set_delay(SensorsManager* thiz, int handle, int64_t ns);
int sensors_manager_poll(SensorsManager* thiz, sensors_event_t* data, int count);

void sensors_manager_destroy(SensorsManager* thiz);

#endif // SENSORS_MANAGER_H
