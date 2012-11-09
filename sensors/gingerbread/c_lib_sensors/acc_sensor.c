#include "acc_sensor.h"
#include "typedef.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define G_SENSOR_DEVICE_STATE "/sys/class/g_sensor/g_sensor_class/device"
#define G_SENSOR_CMD_POWER_ON "on"
#define G_SENSOR_CMD_POWER_OFF "off"
#define G_SENSOR_CMD_START_TIMER "start %u"
#define G_SENSOR_CMD_STOP_TIMER "stop"

#define G_SENSOR_DEFAULT_TIMER 400

typedef struct {
	int fd;
} PrivInfo;

static int acc_sensor_activate(SensorBase* thiz, int handle, int enabled)
{
	return_val_if_fail(thiz != NULL, -1);

	int control_fd = open(G_SENSOR_DEVICE_STATE, O_WRONLY);
	if (control_fd < 0) {
		return -1;
	}

	if (enabled) {
	}
}

SensorBase* acc_sensor_create(const char* devname) 
{
	SensorBase* thiz = (SensorBase*) malloc(sizeof(SensorBase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
	}

	return thiz;
}

#ifdef ACC_SENSOR_TEST

int main(int argc, char** argv)
{
	SensorBase* acc = acc_sensor_create("g_sensor");

	sensor_activate(acc, 1);
	sensor_set_delay(acc, 500);
	
	return 0;
}
#endif // ACC_SENSOR_TEST
