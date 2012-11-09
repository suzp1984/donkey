#include "sensors_manager.h"
#include "sensor_base.h"

#include <stddef.h>
#include <poll.h>
#include <unistd.h>

#define NUM_SENSORS 4
#define WAKE_MESSAGE 'W'
#define WAKE_FD (numFds - 1)

struct _SensorsManager {
	SensorBase* sensors[numSensorDrivers];

	struct pollfd poll_fds[numFds];
	int write_pipe_fd;
};

static int handle_to_driver(int handle) const {
	switch(handle) {
		case ID_A:
			return accelerometer;
		case ID_O:
		case ID_M:
			return akm;
		case ID_P:
			return proximity;
		case ID_L:
			return light;
	}

	return -1;
}

SensorsManager* sensors_manager_create()
{
	SensorsManager* thiz = (SensorsManager*) malloc(sizeof(SensorsManager));

	int wake_fds[2];
	int res;
	if (thiz != NULL) {
		thiz->sensors[accelerometer] = acc_sensor_create("kxtj9_accel");
		thiz->poll_fds[accelerometer].fd = thiz->sensors[accelerometer]->get_fd(thiz->sensors[accelerometer]);
		thiz->poll_fds[accelerometer].events = POLLIN;
		thiz->poll_fds[accelerometer].revents = 0;

		thiz->sensors[akm] = NULL;

		res = pipe(wake_fds);
		fcntl(wake_fds[0], F_SETFL, O_NONBLOCK);
		fcntl(wake_fds[1], F_SETFL, O_NONBLOCK);
		thiz->write_pipe_fd = wake_fds[1];
		
		thiz->poll_fds[WAKE_FD].fd = wake_fds[0];
		thiz->poll_fds[WAKE_FD].events = POLLIN;
		thiz->poll_fds[WAKE_FD].revents = 0;
	}
}

int sensors_manager_activate(SensorsManager* thiz, int handle, int enabled)
{
	return_val_if_fail(thiz != NULL, -1);
	int ret = -1;
	char wakechar = WAKE_MESSAGE;

	int driver = handle_to_driver(handle);
	return_val_if_fail(driver != -1, -1);

	// return 0 if success,
	ret = thiz->sensors[driver]->activate(thiz->sensors[driver], handle, enabled);

	if (enabled && !ret) {
		write(thiz->write_pipe_fd, &wakechar, 1);
	}

	return ret;
}


int sensors_manager_set_delay(SensorsManager* thiz, int handle, int64_t ns)
{
	return_val_if_fail(thiz != NULL, -1);

	int driver = handle_to_driver(handle);
	return_val_if_fail(driver != -1, -1);

	return_val_if_fail(thiz->sensors[driver] != NULL, -1);

	return thiz->sensors[driver]->set_delay(thiz->sensors[driver], handle, ns);
}

int sensors_manager_poll(SensorsManager* thiz, sensors_event_t* data, int count)
{
	return_val_if_fail(thiz != NULL, -1);

	int nbEvents = 0;
	int n = 0;

	int i = 0;
	do {
		for (i = 0; count && i < numSensorDrivers; i++) {
			if ((thiz->poll_fds[i].revents & POLLIN) || 
					(thiz->sensors[i]->has_pending_events(thiz->sensors[i]))) {
				int nb = thiz->sensors[i]->poll(thiz->sensors[i], data, count);
				if (nb < count) {
					thiz->poll_fds[i].revents = 0;
				}
				
				count -= nb;
				nbEvents += nb;
				data += nb;
			}
		}

		if (count) {
			n = poll(thiz->poll_fds, numFds, nbEvents ? 0 : -1);

			if (n < 0) {
				return -1;
			}

			if (thiz->poll_fds[WAKE_FD].revents & POLLIN) {
				char msg;
				int result = read(thiz->poll_fds[WAKE_FD].fd, &msg, 1);
				if (msg == WAKE_MESSAGE) {
					thiz->poll_fds[WAKE_FD].revents = 0;
				}
			}
		}
	} while (n && count);

	return nbEvents;
	//return thiz->sensors[driver]->poll(thiz->sensors[driver], data, count);
}

void sensors_manager_destroy(SensorsManager* thiz)
{
	return_if_fail(thiz != NULL);

	int i = 0;
	for (i = 0; i < numSensorDrivers; i++) {
		sensor_destroy(thiz->sensors[i]);
	}

	close(thiz->poll_fds[WAKE_FD].fd);
	close(thiz->write_pipe_fd);
	// TODO something

	SAFE_FREE(thiz);
}
