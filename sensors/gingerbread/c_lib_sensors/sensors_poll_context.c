#include "sensors_poll_context.h"
#include <hardware/sensors.h>
#include "sensors_manager.h"

typedef int (*SensorActivate)(struct sensors_poll_device_t *dev,
            int handle, int enabled);
typedef int (*SensorSetDelay)(struct sensors_poll_device_t *dev,
            int handle, int64_t ns);
typedef int (*SensorPoll)(struct sensors_poll_device_t *dev,
            sensors_event_t* data, int count);
typedef int (*ModuleClose)(struct hw_device_t* dev);

struct _SensorsPollContext {
	struct sensors_poll_device_t device;  // must be first;

	SensorsManager* sensors_manager;

};

static int sensors_activate(struct sensors_poll_device_t* dev, 
		int handle, int enabled)
{
	SensorsPollContext* ctx = (SensorsPollContext*)dev;
	return_val_if_fail(ctx != NULL, -1);

	return sensors_manager_activate(ctx->sensors_manager, handle, enabled);

}

static int sensors_set_delay(struct sensors_poll_device_t* dev,
		int handle, int64_t ns)
{
	SensorsPollContext* ctx = (SensorsPollContext*)dev;
	return_val_if_fail(ctx != NULL, -1);

	return sensors_manager_set_delay(ctx->sensors_manager, handle, ns);
}

static int sensors_poll(struct sensors_poll_device_t* dev,
		sensors_event_t* data, int count)
{
	SensorsPollContext* ctx = (SensorsPollContext*)dev;
	return_val_if_fail(ctx != NULL, -1);

	return sensors_manager_poll(ctx->sensors_manager, data, count);
}

static int sensors_close(struct hw_device_t* dev)
{
	SensorsPollContext* ctx = (SensorsPollContext*)dev;
		
	sensors_poll_context_destroy(ctx);

	return 0;
}

SensorsPollContext* sensors_poll_context_create(const struct hw_module_t* module)
{
	SensorsPollContext* thiz = (SensorsPollContext*)malloc(sizeof(SensorsPollContext));

	if (thiz != NULL) {
		thiz->device.common.tag = HARDWARE_MODULE_TAG;
		thiz->device.common.version = 0;
		thiz->device.common.module = (struct hw_module_t*)module;
		thiz->device.common.close = NULL;
		thiz->device.activate = sensors_activate;
		thiz->device.setDelay = sensors_set_delay;
		thiz->device.poll = sensors_poll;

		thiz->sensors_manager = sensors_manager_create();
	}

	return thiz;
}

void sensors_poll_context_destroy(SensorsPollContext* thiz)
{
	return_if_fail(thiz != NULL);

	sensors_manager_destroy(thiz->sensors_manager);
	SAFE_FREE(thiz);

	return;
}
