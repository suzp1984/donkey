#include "phone_call_test.h"
#include "ui.h"
#include "minui.h"
#include "engat.h"
#include "engapi.h"
#include "common.h"

#include <stdlib.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define SPRD_PHONECALL_FINISH   "CALL 112 is going on..."

static void phone_call_call_start(TestCase* thiz)
{
	int fd = -1;
	char cmd[32];
	fd = engapi_open(0);
	if (fd < 0) {
		LOGE("%s: open sim1 fail", __func__);
		return;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%d,%d,%s", ENG_AT_NOHANDLE_CMD, 1, "ATD112;");
	LOGE("%s: send cmd %s", __func__, cmd);
	engapi_write(fd, cmd, strlen(cmd));

	memset(cmd, 0, sizeof(cmd));
	engapi_read(fd, cmd, sizeof(cmd));
	LOGE("%s: response=%s", __func__, cmd);

	engapi_close(fd);
}

static void phone_call_call_stop(TestCase* thiz)
{
	int fd;
	char cmd[32];

	fd = engapi_open(0);
	if (fd < 0)
		return;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%d,%d,%s", ENG_AT_NOHANDLE_CMD, 1, "ATH");
	LOGE("%s: send cmd %s", __func__, cmd);
	engapi_write(fd, cmd, strlen(cmd));

	memset(cmd, 0, sizeof(cmd));
	engapi_read(fd, cmd, sizeof(cmd));
	LOGE("%s: response=%s", __func__, cmd);

	engapi_close(fd);
}

static int phone_call_test_run(TestCase* thiz)
{
	ui_screen_clean();
	
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, PHONE_CALL_TEST);
	ui_draw_title(gr_fb_height() / 2, SPRD_PHONECALL_FINISH);
	gr_flip();

	phone_call_call_start(thiz);
	
	thiz->passed = ui_draw_handle_softkey(thiz->name);

	phone_call_call_stop(thiz);

	fk_stop_service("media");
	fk_start_service("media");
	sleep(2);

	return 0;
}

static void phone_call_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* phone_call_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase));

	if (thiz != NULL) {
		thiz->run = phone_call_test_run;
		thiz->destroy = phone_call_test_destroy;

		thiz->name = PHONE_CALL_TEST;
		thiz->passed = passed;
	}

	return thiz;
}
