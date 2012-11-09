#include "rtc_test.h"
#include "ui.h"
#include "minui.h"

#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

typedef struct {
	int thread_run;
} PrivInfo;

static void* rtc_test_thread_run(void* ptr);

static void* rtc_test_thread_run(void* ptr)
{
	TestCase* thiz = (TestCase*)ptr;
	DECLES_PRIV(priv, thiz);

	char timebuf[32];
	time_t timer;
	struct tm* t_tm;
	memset(timebuf, 0, sizeof(timebuf));

	while(priv->thread_run) {
		time(&timer);
		t_tm = localtime(&timer);
		gr_color(0, 0, 0, 255);
		ui_draw_title(gr_fb_height()/2, timebuf);
		memset(timebuf, 0, sizeof(timebuf));
		sprintf(timebuf, "%02d:%02d:%02d", t_tm->tm_hour, t_tm->tm_min, t_tm->tm_sec);
		gr_color(0, 255, 0, 255);
		ui_draw_title(gr_fb_height()/2, timebuf);
		gr_flip();
	}

	return NULL;
}
static int rtc_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	time_t time1;
	time_t time2;
	int ret = 0;
	pthread_t t;

	ui_screen_clean();
	
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, RTC_TEST_CASE);
	gr_flip();

	priv->thread_run = 1;
	pthread_create(&t, NULL, rtc_test_thread_run, (void*)thiz);

	time(&time1);
	sleep(1);
	time(&time2);
	ret = (int)(time2 - time1);

	thiz->passed = (ret == 1) ? 1 : 0;

	ui_quit_with_prompt(RTC_TEST_CASE, thiz->passed);

	priv->thread_run = 0;
	pthread_join(t, NULL);
	
	return 0;
}

static void rtc_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* rtc_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = rtc_test_run;
		thiz->destroy = rtc_test_destroy;

		thiz->name = RTC_TEST_CASE;
		thiz->passed = passed;
		priv->thread_run = 0;
	}

	return thiz;
}
