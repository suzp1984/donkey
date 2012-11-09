#include "multi_touch_test.h"
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

#define SPRD_MULTI_POINT                2
#define ABS_MT_POSITION_X               0x35    /* Center X ellipse position */
#define ABS_MT_POSITION_Y               0x36    /* Center Y ellipse position */
#define ABS_MT_TOUCH_MAJOR              0x30
#define SPRD_BLOCK_WIDTH                20
#define SPRD_MULTI_PRESSED_OFFSET       40
#define SPRD_MULTI_INPUT_DEV            "ctp"

#define POWER_KEYCODE 116

typedef enum { 
	FINGER_1 = 0, 
	FINGER_2,
	POINT_MAX
}FINGER_POINT;

typedef struct _tspoint_t {
	int x;
	int y;
	int pressed;
} tspoint_t;

typedef struct _tsinfo_t {
	tspoint_t point[SPRD_MULTI_POINT];
	tspoint_t touch_point;
} tsinfo_t;

typedef struct {
	tsinfo_t tsinfo;
	int thread_run;
	int ts_success;
	int force_quit;
	int sync_flags;
	int x_pressed;
	int y_pressed;
} PrivInfo;

static void multi_touch_test_show(TestCase* thiz);
static void multi_touch_test_handle_event(TestCase* thiz, struct input_event* event);
static void multi_touch_test_pressed(TestCase* thiz);
static void* multi_touch_test_wait_key(void* ptr);

static void* multi_touch_test_wait_key(void* ptr)
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

static void multi_touch_test_pressed(TestCase* thiz) 
{
	DECLES_PRIV(priv, thiz);
	int i = 0;
	tspoint_t block_start;
	tspoint_t point_pressed;

	for (i = 0; i < POINT_MAX; i++) {
		block_start = priv->tsinfo.point[i];
		point_pressed = priv->tsinfo.touch_point;

		if ((block_start.x < point_pressed.x) && (point_pressed.x < block_start.x + SPRD_MULTI_PRESSED_OFFSET) &&
				(block_start.y < point_pressed.y) && (point_pressed.y < block_start.y + SPRD_MULTI_PRESSED_OFFSET)) {
			priv->tsinfo.point[i].pressed = 1;
			break;
		}
	}

	multi_touch_test_show(thiz);
}

static void multi_touch_test_handle_event(TestCase* thiz, struct input_event* event)
{
	DECLES_PRIV(priv, thiz);
	int i = 0;

	if (event->type == EV_SYN) {
		if (event->code == 2) {
			priv->sync_flags++;
			if (priv->sync_flags >= POINT_MAX) {
				for(i = 0; i < POINT_MAX; i++) {
					priv->tsinfo.point[i].pressed = 0;
				}
			}
		}
	}

	if (event->type == EV_ABS) {
		switch(event->code) {
			case ABS_MT_POSITION_X:
				priv->x_pressed = 1;
				priv->tsinfo.touch_point.x = event->value;
				break;
			case ABS_MT_POSITION_Y:
				priv->y_pressed = 1;
				priv->tsinfo.touch_point.y = event->value;
				break;
			case ABS_MT_TOUCH_MAJOR:
				if (event->value == 0) {
					priv->x_pressed = 0;
					priv->y_pressed = 0;
					for (i = 0; i < POINT_MAX; i++) {
						priv->tsinfo.point[i].pressed = 0;
					}
				}
				break;
			default:
				break;
		}
	}

	if (priv->x_pressed && priv->y_pressed) {
		priv->x_pressed = 0;
		priv->y_pressed = 0;
		priv->sync_flags = 0;
		multi_touch_test_pressed(thiz);
	}
}

static void multi_touch_test_show(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	ui_screen_clean();
	int i = 0;
	int x;
	int y;
	int width;
	int height;

	gr_color(0, 255, 0, 255);
	ui_draw_title(UI_TITLE_START_Y, MULTI_TOUCH_TEST_CASE);

	priv->ts_success = 1;
	for (i = 0; i < POINT_MAX; i++) {
		if (priv->tsinfo.point[i].pressed == 0) {
			priv->ts_success = 0;
			break;
		}
	}

	if (priv->ts_success == 1) {
		goto out;
	}

	for (i = 0; i < POINT_MAX; i++) {
		x = priv->tsinfo.point[i].x;
		y = priv->tsinfo.point[i].y;
		width = priv->tsinfo.point[i].x + SPRD_BLOCK_WIDTH;
		height = priv->tsinfo.point[i].y + SPRD_BLOCK_WIDTH;
		gr_color(64, 96, 255, 255);
		gr_fill(x, y, width, height);
	}

out:
	gr_flip();
}

static void* multi_touch_test_thread(void* ptr)
{
	TestCase* thiz = (TestCase*)ptr;
	DECLES_PRIV(priv, thiz);

	fd_set rfds;
	struct input_event event;
	struct timeval timeout;
	int n;

	int ts_fd = open_input(SPRD_MULTI_INPUT_DEV, O_RDONLY);
	if (ts_fd < 0) {
		LOGE("%s: fail to get input dev %s", __func__, SPRD_MULTI_INPUT_DEV);
		return NULL;
	}

	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	while(priv->thread_run == 1) {

		if (priv->ts_success == 1) {
			break;
		}

		FD_ZERO(&rfds);
		FD_SET(ts_fd, &rfds);
		
		n = select(ts_fd + 1, &rfds, NULL, NULL, &timeout);
		if (n < 0) {
			LOGE("%s: error from select(%d): %s", __func__,
					ts_fd, strerror(errno));
		}

		if (FD_ISSET(ts_fd, &rfds)) {
			n = read(ts_fd, &event, sizeof(event));
			if (n == sizeof(event)) {
				multi_touch_test_handle_event(thiz, &event);
			}
		}
	}

	return NULL;
}

static int multi_touch_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	pthread_t t;
	pthread_t m;
	int i = 0;

	priv->ts_success = 0;
	priv->force_quit = 0;
	priv->sync_flags = 0;
	priv->x_pressed = 0;
	priv->y_pressed = 0;

	for (i = 0; i < POINT_MAX; i++) {
		priv->tsinfo.point[i].pressed = 0;
	}
	multi_touch_test_show(thiz);

	priv->thread_run = 1;
	pthread_create(&t, NULL, multi_touch_test_thread, (void*)thiz);
	pthread_create(&m, NULL, multi_touch_test_wait_key, (void*)thiz);

	while(priv->ts_success == 0 && priv->force_quit == 0) {
		sleep(1);
	}

	priv->thread_run = 0;
	pthread_join(t, NULL);

	ui_draw_prompt_noblock(priv->ts_success);
	ui_quit_by_result(thiz->name, priv->ts_success);
	thiz->passed = priv->ts_success;

	pthread_join(m, NULL);

	return 0;
}

static void multi_touch_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* multi_touch_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = multi_touch_test_run;
		thiz->destroy = multi_touch_test_destroy;

		thiz->name = MULTI_TOUCH_TEST_CASE;
		thiz->passed = passed;
		priv->thread_run = 0;
		priv->ts_success = 0;
		priv->force_quit = 0;
		priv->sync_flags = 0;
		priv->x_pressed = 0;
		priv->y_pressed = 0;

		priv->tsinfo.point[FINGER_1].x = gr_fb_width()/2 - SPRD_BLOCK_WIDTH/2;
		priv->tsinfo.point[FINGER_1].y = gr_fb_height()/2 - SPRD_BLOCK_WIDTH/2;

		priv->tsinfo.point[FINGER_2].x = gr_fb_width()/4 - SPRD_BLOCK_WIDTH/2;
		priv->tsinfo.point[FINGER_2].y = gr_fb_height()/4 - SPRD_BLOCK_WIDTH/2;
	}

	return thiz;
}
