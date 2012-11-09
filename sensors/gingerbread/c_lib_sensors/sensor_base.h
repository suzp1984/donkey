#ifndef SENSOR_BASE_H
#define SENSOR_BASE_H

#include <hardware/sensors.h>
#include "typedef.h"
#include "common.h"

struct _SensorBase;
typedef struct _SensorBase SensorBase;

typedef int (*SensorBaseActivate)(SensorBase* thiz, int handle, int enabled);
typedef int (*SensorBaseSetDelay)(SensorBase* thiz, int handle, int64_t ns);
typedef int (*SensorBasePoll)(SensorBase* thiz, sensors_event_t* data, int count);
typedef int (*SensorBaseGetFd)(SensorBase* thiz);
typedef int (*SensorBaseHasPendingEvents)(SensorBase* thiz);
typedef void (*SensorBaseDestroy)(SensorBase* thiz);

struct _SensorBase {
	SensorBaseActivate activate;
	SensorBaseSetDelay set_delay;
	SensorBasePoll poll;
	SensorBaseGetFd get_fd;
	SensorBaseHasPendingEvents has_pending_events;
	SensorBaseDestroy destroy;

	char priv[1];
};

static inline int sensor_activate(SensorBase* thiz, int handle, int enabled)
{
	return_val_if_fail(thiz != NULL && thiz->activate != NULL, -1);

	return thiz->activate(thiz, handle, enabled);
}

static inline int sensor_set_delay(SensorBase* thiz, int handle, int64_t ns)
{
	return_val_if_fail(thiz != NULL && thiz->set_delay != NULL, -1);

	return thiz->set_delay(thiz, handle, ns);
}

static inline int sensor_poll(SensorBase* thiz, sensors_event_t* data, int count)
{
	return_val_if_fail(thiz != NULL && thiz->poll != NULL, -1);

	return thiz->poll(thiz, data, count);
}

static inline int sensor_get_fd(SensorBase* thiz)
{
	return_val_if_fail(thiz != NULL && thiz->get_fd != NULL, -1);

	return thiz->get_fd(thiz);
}

static inline int sensor_has_pending_events(SensorBase* thiz)
{
	return_val_if_fail(thiz != NULL && thiz->has_pending_events != NULL, 0);

	return thiz->has_pending_events(thiz);
}

static inline int sensor_destroy(SensorBase* thiz)
{
	return_if_fail(thiz != NULL && thiz->destroy != NULL);

	return thiz->destroy(thiz);
}

#endif // SENSOR_BASE_H
