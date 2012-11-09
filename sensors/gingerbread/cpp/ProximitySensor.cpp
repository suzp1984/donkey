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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>

#include "ProximitySensor.h"

/*****************************************************************************/

ProximitySensor::ProximitySensor()
    : SensorBase(NULL, "p_sensor"),
      mEnabled(0),
      mInputReader(4)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_P;
    mPendingEvent.type = SENSOR_TYPE_PROXIMITY;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    open_device();

    if (!mEnabled) {
        close_device();
    }
}

ProximitySensor::~ProximitySensor() {
}

int ProximitySensor::enable(int32_t, int en) {
    int newState = en ? 1 : 0;
    int err = 0;
    int control_fd = -1;
    char cmd[32];
    if (newState != mEnabled) {
        if (!mEnabled) {
            open_device();
        }

        control_fd = open(P_SENSOR_DEVICE_STATE, O_WRONLY);
        if(control_fd < 0) {
        	LOGE("in func %s: cannot open %s.", __FUNCTION__, P_SENSOR_DEVICE_STATE);
        	return -1;
		}
		sprintf(cmd, en ? "1" : "0");
		write(control_fd, cmd, sizeof(cmd));
        
        mEnabled = en ? 1 : 0;
        if (!mEnabled) {
            close_device();
        }
    }
    return err;
}

int ProximitySensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            if (event->code == EVENT_TYPE_PROXIMITY) {
                mPendingEvent.distance = indexToValue(event->value);
            }
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            LOGE("ProximitySensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

float ProximitySensor::indexToValue(size_t index) const
{
    return index * PROXIMITY_THRESHOLD_CM;
}
