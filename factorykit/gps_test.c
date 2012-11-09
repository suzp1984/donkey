#include "gps_test.h"
#include "ui.h"
#include "minui.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define GPS_TEST_FILE "/data/gpstest.txt"

#define GPS_TEST_PASS "Gps Test Pass"
#define GPS_TEST_FAIL "Gps Test Fail"

typedef struct {
	int thread_run;
	int passed;
} PrivInfo;

static void* gps_test_thread_run(void* ptr);
static void gps_test_show(TestCase* thiz);

static void* gps_test_thread_run(void* ptr)
{
	TestCase* thiz = (TestCase*)ptr;
	DECLES_PRIV(priv, thiz);

	while(priv->thread_run) {
		gps_test_show(thiz);
		sleep(1);
	}

	return NULL;
}

static void gps_test_show(TestCase* thiz) 
{
	DECLES_PRIV(priv, thiz);

	char test_result[100];
	int fd = -1;
	int n = 0;
	fd = open(GPS_TEST_FILE, O_RDONLY);

	if (fd < 0) {
		goto out;
	}

	memset(test_result, 0, sizeof(test_result));
	n = read(fd, test_result, sizeof(test_result));
	LOGE("%s: %s", __func__, test_result);
	if (n > 0) {
		priv->passed = 1;
	}
	close(fd);

out:
	if (priv->passed) {
		gr_color(0, 0, 0, 255);
		ui_draw_title(gr_fb_height()/2, GPS_TEST_FAIL);
		gr_color(0, 255, 0, 255);
		ui_draw_title(gr_fb_height()/2, GPS_TEST_PASS);
	} else {
		gr_color(0, 0, 0, 255);
		ui_draw_title(gr_fb_height()/2, GPS_TEST_PASS);
		gr_color(255, 0, 0, 255);
		ui_draw_title(gr_fb_height()/2, GPS_TEST_FAIL);
	}

	ui_draw_prompt_noblock(priv->passed);
}

static int gps_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	pthread_t t;
	priv->passed = 0;
	remove(GPS_TEST_FILE);
	system("gsd4t_testmode4_app 7 20 &");

	ui_screen_clean();

	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, GPS_TEST_CASE);
	gr_flip();

	priv->thread_run = 1;
	pthread_create(&t, NULL, gps_test_thread_run, (void*)thiz);

	ui_wait_anykey();
	thiz->passed = priv->passed;
	ui_quit_by_result(thiz->name, thiz->passed);

	priv->thread_run = 0;
	pthread_join(t, NULL);
	return 0;
}

static void gps_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* gps_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = gps_test_run;
		thiz->destroy = gps_test_destroy;

		thiz->name = GPS_TEST_CASE;
		thiz->passed = passed;
		priv->thread_run = 0;
		priv->passed = 0;
	}

	return thiz;
}
