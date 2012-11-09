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

#ifndef ANDROID_ACCELEROMETER_SENSOR_H
#define ANDROID_ACCELEROMETER_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "nusensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

#define G_SENSOR_DEVICE_STATE "/sys/class/g_sensor/g_sensor_class/device"
#define G_SENSOR_CMD_POWER_ON "on"
#define G_SENSOR_CMD_POWER_OFF "off"
#define G_SENSOR_CMD_START_TIMER "start %u"
#define G_SENSOR_CMD_STOP_TIMER "stop"

#define G_SENSOR_DEFAULT_TIMER 400

struct input_event;

class AccelerometerSensor : public SensorBase {
	enum {
		Accelerometer = 0,
		Orientation   = 1,
		numSensors
	};

    int mEnabled;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvents[numSensors];
    uint32_t mPendingMask;

public:
            AccelerometerSensor();
    virtual ~AccelerometerSensor();
    virtual int readEvents(sensors_event_t* data, int count);
    virtual int enable(int32_t handle, int enabled);
    void processEvent(int code, int value);
};

/*****************************************************************************/

#endif  // ANDROID_ACCELEROMETER_SENSOR_H
