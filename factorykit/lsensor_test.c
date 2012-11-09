#include "lsensor_test.h"
#include "ui.h"
#include "minui.h"
#include "common.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <linux/input.h>
#include <poll.h>
#include <pthread.h>
#include <sys/select.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define SPRD_PLS_CTL            "/sys/class/l_sensor/l_sensor_class/als_on"
#define SPRD_PLS_INPUT_DEV      "l_sensor"
#define SPRD_PLS_LIGHT_THRESHOLD    15

typedef struct {
	int thread_run;
	int value;
	int passed;
} PrivInfo;

static int lsensor_test_enable(TestCase* thiz, int enable);
static int lsensor_test_read_event(TestCase* thiz, struct input_event* event);
static void* lsensor_test_thread_run(void* ptr);
static void lsensor_test_show(TestCase* thiz);

static void lsensor_test_show(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	char buffer[64];

	ui_screen_clean();
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, LSENSOR_TEST_CASE);

	if (priv->value > SPRD_PLS_LIGHT_THRESHOLD) {
		gr_color(0, 255, 0, 255);
	} else {
		gr_color(255, 0, 0, 255);
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "Light Lux: %d", priv->value);

	ui_draw_title(gr_fb_height()/2, buffer);
	ui_draw_prompt_noblock(priv->passed);

	gr_flip();
}

static int lsensor_test_read_event(TestCase* thiz, struct input_event* event)
{
	DECLES_PRIV(priv, thiz);

	if (event->type == EV_ABS) {
		switch(event->code) {
			case ABS_THROTTLE:
				priv->value = event->value;
				break;
		}
	}

	if (priv->value > SPRD_PLS_LIGHT_THRESHOLD) {
		priv->passed = 1;
	}

	return 0;
}

static void* lsensor_test_thread_run(void* ptr)
{
	TestCase* thiz = (TestCase*)ptr;
	DECLES_PRIV(priv, thiz);

	int ret;
	int n;
	int nread;
	int input_fd;
	fd_set rfds;
	struct input_event event;
	struct timeval timeout;

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	lsensor_test_enable(thiz, 1);

	input_fd = open_input(SPRD_PLS_INPUT_DEV, O_RDONLY);

	while(priv->thread_run) {
		FD_ZERO(&rfds);
		FD_SET(input_fd, &rfds);
		n = select(input_fd + 1, &rfds, NULL, NULL, &timeout);

		if (FD_ISSET(input_fd, &rfds)) {
			nread = read(input_fd, &event, sizeof(event));
			if (nread == sizeof(event)) {
				lsensor_test_read_event(thiz, &event);
				lsensor_test_show(thiz);
			}
		}

	}

	lsensor_test_enable(thiz, 0);

	return NULL;
}

static int lsensor_test_enable(TestCase* thiz, int enable)
{
	int fd;
	char buffer[8];

	fd = open(SPRD_PLS_CTL, O_RDWR);

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", enable);

	if (fd >= 0) {
		write(fd, buffer, sizeof(buffer));
		close(fd);
	} else {
		return -1;
	}

	return 0;
}

static int lsensor_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	priv->value = 0;
	priv->passed = 0;
	pthread_t t;

	lsensor_test_show(thiz);

	priv->thread_run = 1;
	pthread_create(&t, NULL, lsensor_test_thread_run, (void*)thiz);

	ui_wait_anykey();

	thiz->passed = priv->passed;
	ui_quit_by_result(thiz->name, thiz->passed);
	priv->thread_run = 0;
	pthread_join(t, NULL);

	return 0;
}

static void lsensor_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* lsensor_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = lsensor_test_run;
		thiz->destroy = lsensor_test_destroy;

		thiz->name = LSENSOR_TEST_CASE;
		thiz->passed = passed;
		priv->thread_run = 0;
		priv->value = 0;
		priv->passed = 0;
	}

	return thiz;
}
