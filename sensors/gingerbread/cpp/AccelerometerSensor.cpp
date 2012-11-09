#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>

#include "AccelerometerSensor.h"

AccelerometerSensor::AccelerometerSensor() 
	: SensorBase(NULL, "g_sensor"),
		mEnabled(0),
		mInputReader(8)
{
	memset(mPendingEvents, 0, sizeof(mPendingEvents));

	mPendingEvents[Accelerometer].version = sizeof(sensors_event_t);
    mPendingEvents[Accelerometer].sensor = ID_A;
    mPendingEvents[Accelerometer].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents[Accelerometer].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

	mPendingEvents[Orientation].version = sizeof(sensors_event_t);
	mPendingEvents[Orientation].sensor = ID_O;
	mPendingEvents[Orientation].type = SENSOR_TYPE_ORIENTATION;
	mPendingEvents[Orientation].orientation.status = SENSOR_STATUS_ACCURACY_HIGH;

    open_device();

    if (!mEnabled) {
        close_device();
    }
}

AccelerometerSensor::~AccelerometerSensor() {
}

int AccelerometerSensor::enable(int32_t, int en) {
	int flags = en ? 1 : 0;
	int control_state_fd = -1;
	if (flags != mEnabled) {
		if (!mEnabled) {
			open_device();
		}

		control_state_fd = open(G_SENSOR_DEVICE_STATE, O_WRONLY);
		if(control_state_fd < 0) {
			LOGE("in func %s: cannot open %s.", __FUNCTION__, G_SENSOR_DEVICE_STATE);
			return -1;
		}

		if(en) {
			char cmd[32];
			sprintf(cmd, G_SENSOR_CMD_START_TIMER, (uint)G_SENSOR_DEFAULT_TIMER);
			write(control_state_fd, cmd, sizeof(cmd));
			write(control_state_fd, G_SENSOR_CMD_POWER_ON, sizeof(G_SENSOR_CMD_POWER_ON));
		} else {
			write(control_state_fd, G_SENSOR_CMD_STOP_TIMER, sizeof(G_SENSOR_CMD_STOP_TIMER));
			write(control_state_fd, G_SENSOR_CMD_POWER_OFF, sizeof(G_SENSOR_CMD_POWER_OFF));
		}

		mEnabled = en ? 1 : 0;
		if (!mEnabled) {
			close_device();
		}
	}
	return 0;
}

// TODO readEvents
int AccelerometerSensor::readEvents(sensors_event_t* data, int count) {
	if(count < 1) {
		return -EINVAL;
	}

	ssize_t n = mInputReader.fill(data_fd);
	if(n < 0) {
		return n;
	}

	int numEventReceived = 0;
	input_event const* event;

	while(count && mInputReader.readEvent(&event)) {
		int type = event->type;
		if(type == EV_ABS) {
			processEvent(event->code, event->value);
			mInputReader.next();
		} else if (type == EV_SYN) {
			int64_t time = timevalToNano(event->time);
			for (int j = 0; count && mPendingMask && j < numSensors; j++) {
				if(mPendingMask & (1<<j)) {
					mPendingMask &= ~(1<<j);
					mPendingEvents[j].timestamp = time;
					if(mEnabled & (1<<j)) {
						*data++ = mPendingEvents[j];
						count--;
						numEventReceived++;
					}
				}
			}
			if(!mPendingMask) {
				mInputReader.next();
			}
		} else {
			LOGE("Accelerometer: unknown event(type=%d, code=%d)",
					type, event->code);
			mInputReader.next();
		}
	}
	
	return numEventReceived;
}

void AccelerometerSensor::processEvent(int code, int value) {
	switch (code) {
		case EVENT_TYPE_ACCEL_X:
			mPendingMask |= 1<<Accelerometer;
			mPendingEvents[Accelerometer].acceleration.x = value * CONVERT_A_X;
			break;
		case EVENT_TYPE_ACCEL_Y:
			mPendingMask |= 1<<Accelerometer;
			mPendingEvents[Accelerometer].acceleration.y = value * CONVERT_A_Y;
			break;
		case EVENT_TYPE_ACCEL_Z:
			mPendingMask |= 1<<Accelerometer;
			mPendingEvents[Accelerometer].acceleration.z = value * CONVERT_A_Z;
			break;
	}
}
