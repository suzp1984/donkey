#include "rfcali_test_case.h"
#include "ui.h"
#include "minui.h"

#define LOG_TAG "engtest"
#include <utils/Log.h>

#include "engapi.h"
#include "engat.h"
#include <string.h>

typedef struct {
	int start;
} PrivInfo;

static int rfcali_test_run(TestCase* thiz)
{
	int fd;
	int i;
	int start_y = 50;
	char cmd[1024];
	char buffer[128];
	char* ptr;
	char* start_ptr;
	char* end_ptr;
	int width = gr_fb_width() / CHAR_WIDTH - 2;
	int height = gr_fb_height() / CHAR_HEIGHT - 2;
	int bits = 0;

	ui_screen_clean();
	
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, RFCALI_TEST_CASE);

	fd = engapi_open(0);
	LOGD("rfcali is run");
	if (fd < 0) 
		return 0;

	do {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "%d,%d,%d,%d,%d", ENG_AT_SGMR, 3, 0, 0, 3);
		engapi_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		engapi_read(fd, cmd, sizeof(cmd));
	} while (strstr(cmd, "desterr") != NULL);

	start_ptr = cmd;
	end_ptr = cmd + strlen(cmd);

	i = 0;
	while(start_ptr < end_ptr) {
		ptr = start_ptr;
		while(*ptr != 0x0d && *ptr != 0x0a)
			ptr++;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, ptr - start_ptr + 1, "%s", start_ptr);
		LOGD("%s: %s", __func__, buffer);
		if (strstr(buffer, "uncalibrated") == NULL && strstr(buffer, "calibrated") != NULL) {
			gr_color(0, 255, 0, 255);
			bits++;
		} else {
			gr_color(255, 255, 255, 255);
		}

		if (buffer[0] != '\0') {
			gr_text(20,  start_y + i*CHAR_HEIGHT + 3, buffer);
		}

		while(*ptr == 0x0a || *ptr == 0x0d) 
			ptr++;

		start_ptr = ptr;
		i++;
	}
	
	engapi_close(fd);

	gr_flip();

	ui_quit_with_prompt((const char*)thiz->name, bits);
	thiz->passed = bits > 0 ? 1 : 0;
	return 0;
}

static void rfcali_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* rfcali_test_case_create(int passed)
{
	TestCase* thiz = (TestCase*) malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		thiz->run = rfcali_test_run;
		thiz->destroy = rfcali_test_destroy;

		thiz->name = RFCALI_TEST_CASE;
		thiz->passed = passed;
	}

	return thiz;
}
