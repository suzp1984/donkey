#include "tp_test.h"
#include "ui.h"
#include "minui.h"
#include "common.h"

#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <linux/input.h>
#include <string.h>
#include <errno.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define SPRD_TS_OFFSET              20
#define SPRD_BLOCK_WIDTH            10
#define SPRD_TS_INPUT_DEV           "ctp"
#define SPRD_TS_PRESSED_OFFSET      25

#define SPRD_WIDTH_POINTS 	        20
#define SPRD_HEIGHT_POINTS			25

#define POWER_KEYCODE 116
#define ABS_MT_POSITION_X           0x35    /* Center X ellipse position */
#define ABS_MT_POSITION_Y           0x36    /* Center Y ellipse position */

typedef struct _tspoint_t {
	int x;
	int y;
	int pressed;
} tspoint_t;

typedef struct _tsinfo_t {
	tspoint_t point[SPRD_WIDTH_POINTS * 2 + SPRD_HEIGHT_POINTS * 2 - 4];
	tspoint_t touch_point;
} tsinfo_t;

typedef struct {
	tsinfo_t tsinfo;
	int thread_run;
	int ts_success;
	int x_pressed;
	int y_pressed;
	int force_quit;
} PrivInfo;

static int tp_test_show(TestCase* thiz);
static int tp_test_handle_event(TestCase* thiz, struct input_event* event);
static int tp_test_init(TestCase* thiz);

static int tp_test_init(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	int i = 0;
	int width = gr_fb_width();
	int height = gr_fb_height();

	int max_points = SPRD_WIDTH_POINTS * 2 + SPRD_HEIGHT_POINTS * 2 - 4;

	memset(&(priv->tsinfo), 0, sizeof(priv->tsinfo));
	// top line
	for (i = 0; i < SPRD_WIDTH_POINTS; i++) {
		priv->tsinfo.point[i].x = i * width / SPRD_WIDTH_POINTS;
		priv->tsinfo.point[i].y = 0;
	}

	//bottom line
	for (i = 0; i < SPRD_WIDTH_POINTS; i++) {
		priv->tsinfo.point[i + SPRD_WIDTH_POINTS].x = i * width / SPRD_WIDTH_POINTS;
		priv->tsinfo.point[i + SPRD_WIDTH_POINTS].y = height - SPRD_BLOCK_WIDTH;
	}

	// left line
	for (i = 0; i < SPRD_HEIGHT_POINTS - 2; i++) {
		priv->tsinfo.point[i + SPRD_WIDTH_POINTS * 2].x = 0;
		priv->tsinfo.point[i + SPRD_WIDTH_POINTS * 2].y = (i+1) * height / SPRD_HEIGHT_POINTS;
	}

	// right line
	for (i = 0; i < SPRD_HEIGHT_POINTS - 2; i++) {
		priv->tsinfo.point[i + SPRD_WIDTH_POINTS * 2 + SPRD_HEIGHT_POINTS - 2].x = width - SPRD_BLOCK_WIDTH;
		priv->tsinfo.point[i + SPRD_WIDTH_POINTS * 2 + SPRD_HEIGHT_POINTS - 2].y = 
		(i+1) * height / SPRD_HEIGHT_POINTS;
	}

	return 0;
}

static int tp_test_pressed(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int i = 0;
	tspoint_t block_start;
	tspoint_t point_pressed;
	int max_points = SPRD_WIDTH_POINTS * 2 + SPRD_HEIGHT_POINTS * 2 - 4;

	for (i = 0; i < max_points; i++) {
		block_start = priv->tsinfo.point[i];
		point_pressed = priv->tsinfo.touch_point;

		if (block_start.pressed == 1) {
			continue;
		}

		LOGE("%s: start(%d, %d), end(%d, %d), point(%d, %d)", __func__, 
				block_start.x - SPRD_TS_PRESSED_OFFSET , block_start.y - SPRD_TS_PRESSED_OFFSET,
				block_start.x + SPRD_TS_PRESSED_OFFSET, block_start.y + SPRD_TS_PRESSED_OFFSET, 
				point_pressed.x, point_pressed.y);

		if ((block_start.x < point_pressed.x + SPRD_TS_PRESSED_OFFSET) && (point_pressed.x < block_start.x + SPRD_TS_PRESSED_OFFSET) && 
			(block_start.y < point_pressed.y + SPRD_TS_PRESSED_OFFSET) && (point_pressed.y < block_start.y + SPRD_TS_PRESSED_OFFSET)) {
			priv->tsinfo.point[i].pressed = 1;
			break;
		}
	}

	tp_test_show(thiz);
	return 0;
}

static int tp_test_handle_event(TestCase* thiz, struct input_event* event)
{
	DECLES_PRIV(priv, thiz);

	if (event->type == EV_ABS) {
		switch(event->code) {
			case ABS_MT_POSITION_X:
				priv->tsinfo.touch_point.x = event->value;
				priv->x_pressed = 1;
				break;
			case ABS_MT_POSITION_Y:
				priv->tsinfo.touch_point.y = event->value;
				priv->y_pressed = 1;
				break;
		}
	}

	if (priv->x_pressed && priv->y_pressed) {
		priv->x_pressed = 0;
		priv->y_pressed = 0;
		//
		tp_test_pressed(thiz);
	}

	return 0;
}

static int tp_test_check_all_keys(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int i = 0;
	int count = 0;
	int max_points = SPRD_WIDTH_POINTS * 2 + SPRD_HEIGHT_POINTS * 2 - 4;

	for (i = 0; i < max_points; i++) {
		if (priv->tsinfo.point[i].pressed == 1) {
			count++;
		}
	}

	return count;
}

static int tp_test_show(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int i = 0;
	int x;
	int y;
	int width;
	int height;
	int max_points = SPRD_WIDTH_POINTS * 2 + SPRD_HEIGHT_POINTS * 2 - 4;

	ui_screen_clean();

	gr_color(0, 255, 0, 255);
	ui_draw_title(UI_TITLE_START_Y, TP_TEST_CASE);
	
	for (i = 0; i < max_points; i++) {
		if (priv->tsinfo.point[i].pressed == 1)
			continue;

		x = priv->tsinfo.point[i].x;
		y = priv->tsinfo.point[i].y;
		width = priv->tsinfo.point[i].x + SPRD_BLOCK_WIDTH;
		height = priv->tsinfo.point[i].y + SPRD_BLOCK_WIDTH;
		gr_color(64, 96, 255, 255);
		gr_fill(x, y, width, height);
	}

	gr_flip();

	return 0;
}

static void* tp_test_monitor_thread(void* ptr)
{
	TestCase* thiz = (TestCase*)ptr;
	DECLES_PRIV(priv, thiz);
	int key = 0;
	int power_key_count = 0;

	while(priv->ts_success == 0) {
		key = ui_wait_key();
		if (key == POWER_KEYCODE) {
			power_key_count++;
			if (power_key_count > 3) {
				break;
			}
		}
	}

	priv->force_quit = 1;

	return NULL;
}

static void* tp_test_thread(void* ptr)
{
	TestCase* thiz = (TestCase*)ptr;
	DECLES_PRIV(priv, thiz);

	fd_set rfds;
	struct input_event event;
	struct timeval timeout;
	int n;
	int max_points = SPRD_WIDTH_POINTS * 2 + SPRD_HEIGHT_POINTS * 2 - 4;

	int ts_fd = open_input(SPRD_TS_INPUT_DEV, O_RDONLY);
	if (ts_fd < 0) {
		LOGE("%s: fail to get input dev %s", __func__, SPRD_TS_INPUT_DEV);
		return NULL;
	}

	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	while(priv->thread_run == 1) {
		if (tp_test_check_all_keys(thiz) == max_points) {
			priv->ts_success = 1;
			tp_test_show(thiz);
			break;
		}

		FD_ZERO(&rfds);
		FD_SET(ts_fd, &rfds);

		n = select(ts_fd + 1, &rfds, NULL, NULL, &timeout);
		if (n < 0) {
			LOGE("%s: select ts input error: (%s)", __func__,
					strerror(errno));
		}

		if (FD_ISSET(ts_fd, &rfds)) {
			n = read(ts_fd, &event, sizeof(event));	
			if (n == sizeof(event)) {
				tp_test_handle_event(thiz, &event);
			}
		} else {
			//LOGE("%s: fd is not set", __func__);
		}
	}

	return NULL;
}

static int tp_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	pthread_t t;
	pthread_t monitor;

	int key = 0;
	int power_key_count = 0;
	int i = 0;

	priv->ts_success = 0;
	priv->x_pressed = 0;
	priv->y_pressed = 0;
	priv->force_quit = 0;

	tp_test_init(thiz);

	tp_test_show(thiz);

	priv->thread_run = 1;
	pthread_create(&t, NULL, tp_test_thread, (void*)thiz);
	pthread_create(&monitor, NULL, tp_test_monitor_thread, (void*)thiz);

	while(priv->ts_success == 0 && priv->force_quit == 0) {
		sleep(1);
	}

	priv->thread_run = 0;
	pthread_join(t, NULL);

	ui_draw_prompt_noblock(priv->ts_success);
	ui_quit_by_result(thiz->name, priv->ts_success);
	thiz->passed = priv->ts_success;

	pthread_join(monitor, NULL);

	return 0;
}

static void tp_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* tp_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = tp_test_run;
		thiz->destroy = tp_test_destroy;

		thiz->name = TP_TEST_CASE;
		thiz->passed = passed;
		memset(&(priv->tsinfo), 0, sizeof(priv->tsinfo));

		priv->thread_run = 0;
		priv->ts_success = 0;
		priv->x_pressed = 0;
		priv->y_pressed = 0;
		priv->force_quit = 0;
	}

	return thiz;
}

