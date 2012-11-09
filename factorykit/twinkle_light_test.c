#include "twinkle_light_test.h"
#include "ui.h"
#include "minui.h"
#include "common.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define TWINKLE_LIGHT_PATH "sys/class/leds/led-light/brightness"

static int twinkle_light_test_run(TestCase* thiz)
{
	int fd = -1;
	ui_screen_clean();

	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, TWINKLE_LIGHT_TEST_CASE);

	fd = open(TWINKLE_LIGHT_PATH, O_RDWR);

	if (fd >= 0) {
		LOGE("%s: open twinkle light", __func__);
		write(fd, "255", 3);
	}

	thiz->passed = ui_draw_handle_softkey(thiz->name);

	if (fd >= 0) {
		write(fd, "0", 1);
		close(fd);
	}

	return 0;
}

static void twinkle_light_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* twinkle_light_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase));

	if (thiz != NULL) {
		thiz->run = twinkle_light_test_run;
		thiz->destroy = twinkle_light_test_destroy;

		thiz->name = TWINKLE_LIGHT_TEST_CASE;
		thiz->passed = passed;
	}

	return thiz;
}
