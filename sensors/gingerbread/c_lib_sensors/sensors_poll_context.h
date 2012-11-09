#ifndef SENSORS_POLL_CONTEXT_H
#define SENSORS_POLL_CONTEXT_H

#include <hardware/sensors.h>

struct _SensorsPollContext;
typedef struct _SensorsPollContext SensorsPollContext;


SensorsPollContext* sensors_poll_context_create(const struct hw_module_t* module);

void sensors_poll_context_destroy(SensorsPollContext* thiz);

#endif // SENSORS_POLL_CONTEXT_H
