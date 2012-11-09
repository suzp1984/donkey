#include "psensor_test.h"
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

#define SPRD_PLS_CTL            "/sys/class/p_sensor/p_sensor_class/prox_on"

#define SPRD_PLS_INPUT_DEV      "p_sensor"
#define SPRD_PSENSOR_NEAR       "Proximity Sensor Near"
#define SPRD_PSENSOR_FAR        "Proximity Sensor Far"

typedef struct {
	int thread_run;
	int value;
	int change_count;
} PrivInfo;

static int psensor_test_enable(TestCase* thiz, int enable);
static void* psensor_test_thread_run(void* ptr);
static void psensor_test_show(TestCase* thiz);

static void psensor_test_show(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	ui_screen_clean();
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, PSENSOR_TEST_CASE);

	if (priv->change_count > 1) {
		gr_color(0, 255, 0, 255);
	} else {
		gr_color(255, 0, 0, 255);
	}

	if (priv->value > 0) {
		ui_draw_title(gr_fb_height()/2, SPRD_PSENSOR_FAR);
	} else {
		ui_draw_title(gr_fb_height()/2, SPRD_PSENSOR_NEAR);
	}

	ui_draw_prompt_noblock(priv->change_count);
	gr_flip();
}

static void* psensor_test_thread_run(void* ptr)
{
	TestCase* thiz = (TestCase*)ptr;
	DECLES_PRIV(priv, thiz);
	int ret;
	int input_fd;
	int n;
	int nread;
	struct input_event event;
	struct timeval timeout;
	fd_set rfds;

	psensor_test_enable(thiz, 1);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	input_fd = open_input(SPRD_PLS_INPUT_DEV, O_RDONLY);

	while(priv->thread_run == 1) {
		FD_ZERO(&rfds);
		FD_SET(input_fd, &rfds);
		n = select(input_fd + 1, &rfds, NULL, NULL, &timeout);

		if (FD_ISSET(input_fd, &rfds)) {
			nread = read(input_fd, &event, sizeof(event));
			if (nread == sizeof(event)) {
				if (event.type == EV_ABS) {
					switch (event.code) {
						case ABS_DISTANCE:
							priv->value = event.value;
							priv->change_count++;
							psensor_test_show(thiz);
							break;
					}
				}
			}
		}
	}

	psensor_test_enable(thiz, 0);

	return NULL;
}

static int psensor_test_enable(TestCase* thiz, int enable)
{
	int fd;
	char buffer[8];
	
	fd = open(SPRD_PLS_CTL, O_RDWR);

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", enable);

	if (fd > 0) {
		write(fd, buffer, strlen(buffer));
		close(fd);
	}

	return 0;
}

static int psensor_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	thiz->passed = 0;
	priv->value = 0;
	priv->change_count = 0;

	pthread_t t;
	psensor_test_show(thiz);
	priv->thread_run = 1;
	pthread_create(&t, NULL, psensor_test_thread_run, (void*)thiz);

	ui_wait_anykey();

	if (priv->change_count > 1) {
		thiz->passed = 1;
	}

	ui_quit_by_result(thiz->name, thiz->passed);

	priv->thread_run = 0;
	pthread_join(t, NULL);

	return 0;
}

static void psensor_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* psensor_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = psensor_test_run;
		thiz->destroy = psensor_test_destroy;

		thiz->name = PSENSOR_TEST_CASE;
		thiz->passed = passed;
		priv->thread_run = 0;
		priv->value = 0;
		priv->change_count = 0;
	}

	return thiz;
}
