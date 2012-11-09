#ifndef ACC_SENSOR_H
#define ACC_SENSOR_H

#include "sensor_base.h"

SensorBase* acc_sensor_create(const char* devname);

#endif // ACC_SENSOR_H
