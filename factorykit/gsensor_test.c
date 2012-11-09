#include "gsensor_test.h"
#include "ui.h"
#include "minui.h"
#include "common.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/input.h>
#include <poll.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define GSENSOR_NAME "kxtj9_accel"
#define SPRD_GSENSOR_DEV                    "/sys/class/g_sensor/g_sensor_class/device"
#define GSENSOR_PASSED	"Gsensor Passed"
#define GSENSOR_FAILED  "Gsensor Failed"

#define G_SENSOR_CONVERT        0.00981f    //0.00981f
#define AXIS_X (1 << 0)
#define AXIS_Y (1 << 1)
#define AXIS_Z (1 << 2)
#define SENSORS_AXIS_ALL (AXIS_X | AXIS_Y | AXIS_Z)

typedef struct {
	int control_fd;
	int thread_run;
} PrivInfo;

static int gsensor_test_open(TestCase* thiz);
static int gsensor_test_close(TestCase* thiz);
static void* gsensor_test_thread_run(void* ptr);
static void gsensor_test_show(TestCase* thiz);

static void gsensor_test_show(TestCase* thiz)
{
	int ret = 0;
	
	if (thiz->passed == 1) {
		gr_color(0, 255, 0, 255);
		ui_draw_title(gr_fb_height()/2, GSENSOR_PASSED);
	} else {
		gr_color(255, 0, 0, 255);
		ui_draw_title(gr_fb_height()/2, GSENSOR_FAILED);
	}

	gr_flip();
}

static void* gsensor_test_thread_run(void* ptr)
{
	TestCase* thiz = (TestCase*)ptr;
	DECLES_PRIV(priv, thiz);

	int nr;
	struct input_event event;
	struct pollfd mfd;
	int data[3];
	int g_sensor_axis = 0;

	mfd.fd = open_input(GSENSOR_NAME, O_RDONLY);
	mfd.events = POLLIN;

	gsensor_test_open(thiz);

	while(priv->thread_run) {
		nr = poll(&mfd, 1, -1);
		if (nr < 0) continue;

		if (mfd.revents == POLLIN) {
			int ret = read(mfd.fd, &event, sizeof(event));

			if (ret < (int)sizeof(event)) {
				continue;
			}

			if (event.type == EV_ABS) {
				switch(event.code) {
					case ABS_X:
						data[0] = event.value * G_SENSOR_CONVERT;
						g_sensor_axis |= AXIS_X;
						break;
					case ABS_Y:
						data[1] = event.value * G_SENSOR_CONVERT;
						g_sensor_axis |= AXIS_Y;
						break;
					case ABS_Z:
						data[2] = event.value * G_SENSOR_CONVERT;
						g_sensor_axis |= AXIS_Z;
						break;
					default:
						break;
				}
			} else if (event.type == EV_SYN && (g_sensor_axis & SENSORS_AXIS_ALL)) {
				g_sensor_axis = 0;
				thiz->passed = 1;
				gsensor_test_show(thiz);	
			}
		}
	}

	gsensor_test_close(thiz);

	return NULL;
}

static int gsensor_test_open(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int fd;

	fd = open(SPRD_GSENSOR_DEV, O_RDWR);
	if (fd < 0) {
		LOGE("%s: open %s fail", __func__, SPRD_GSENSOR_DEV);
		return -1;
	}
	priv->control_fd = fd;

	write(fd, "start 400", 9);
	write(fd, "on", 2);

	return 0;
}

static int gsensor_test_close(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	
	if (priv->control_fd < 0) {
		LOGE("%s: cannot get a invalid handle", __func__);
		return -1;
	}

	write(priv->control_fd, "stop", 4);
	write(priv->control_fd, "off", 3);

	close(priv->control_fd);
	priv->control_fd = -1;
	
	return 0;
}

static int gsensor_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	thiz->passed = 0;
	ui_screen_clean();
	pthread_t t;

	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, GSENSOR_TEST_CASE);

	priv->thread_run = 1;
	pthread_create(&t, NULL, gsensor_test_thread_run, (void*)thiz);

	sleep(1);
	gsensor_test_show(thiz);

	ui_quit_with_prompt(GSENSOR_TEST_CASE, thiz->passed);
	priv->thread_run = 0;
	pthread_join(t, NULL);


	return 0;
}

static void gsensor_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* gsensor_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = gsensor_test_run;
		thiz->destroy = gsensor_test_destroy;

		thiz->name = GSENSOR_TEST_CASE;
		thiz->passed = passed;

		priv->control_fd = -1;
		priv->thread_run = 0;
	}

	return thiz;
}
