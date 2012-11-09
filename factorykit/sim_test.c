#include "sim_test.h"
#include "ui.h"
#include "minui.h"

#include <stdlib.h>
#include <stdio.h>
#include "engat.h"
#include "engapi.h"

#define LOG_TAG "factorykit"
#include <utils/Log.h>

static int simtest_poweron(void);
static void* sim_test_poweron_thread_run(void* ptr);

static void* sim_test_poweron_thread_run(void* ptr)
{
	simtest_poweron();

	return NULL;
}

static int simtest_poweron(void) 
{
	int fd;
	char cmd[32];

	//open sim1
	fd = engapi_open(0);
	if (fd < 0) {
		LOGE("%s: open sim1 fail", __func__);
		return -1;
	}

	do {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd , "%d,%d,%s",ENG_AT_NOHANDLE_CMD, 1, "AT+SFUN=2");
		LOGE("%s: send %s", __func__, cmd);
		engapi_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		engapi_read(fd, cmd, sizeof(cmd));
		LOGE("%s: response=%s", __func__, cmd);
		sleep(2);
	} while(strstr(cmd, "OK") == NULL);

	do {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "%d,%d,%s", ENG_AT_NOHANDLE_CMD, 1, "AT+SFUN=4");
		LOGE("%s: send %s", __func__, cmd);
		engapi_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		engapi_read(fd, cmd, sizeof(cmd));
		LOGE("%s: response=%s", __func__, cmd);
		sleep(2);
	} while(strstr(cmd, "OK") == NULL);

	engapi_close(fd);

	// open sim2
	fd = engapi_open(1);
	if (fd < 0) {
		LOGE("%s: open sim2 fail", __func__);
		return -1;
	}

	do {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd , "%d,%d,%s",ENG_AT_NOHANDLE_CMD, 1, "AT+SFUN=2");
		LOGE("%s: send %s", __func__, cmd);
		engapi_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		engapi_read(fd, cmd, sizeof(cmd));
		LOGE("%s: response=%s", __func__, cmd);
		sleep(2);
	} while(strstr(cmd, "OK") == NULL);

	do {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "%d,%d,%s", ENG_AT_NOHANDLE_CMD, 1, "AT+SFUN=4");
		LOGE("%s: send %s", __func__, cmd);
		engapi_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		engapi_read(fd, cmd, sizeof(cmd));
		LOGE("%s: response=%s", __func__, cmd);
		sleep(2);
	} while(strstr(cmd, "OK") == NULL);

	engapi_close(fd);

	return 0;
}

static int simtest_checksim(int type)
{
	int fd, ret = 0;
	char cmd[32];
	char simstatus;
	fd = engapi_open(type);

	if (fd < 0)
		return 0;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%d,%d", ENG_AT_EUICC, 0);
	engapi_write(fd, cmd, strlen(cmd));
	memset(cmd, 0, sizeof(cmd));
	engapi_read(fd, cmd, sizeof(cmd));

	simstatus = cmd[0];
	if (simstatus == '1') {
		ret = 0;
	} else if ((simstatus == '0') || (simstatus == '2')) {
		ret = 1;
	}

	engapi_close(fd);

	return ret;
}

static int sim_test_run(TestCase* thiz)
{
	char sim1_buf[64];
	char sim2_buf[64];

	int sim1_in = simtest_checksim(0);
	int sim2_in = simtest_checksim(1);

	thiz->passed = (sim1_in == 1 && sim2_in == 1) ? 1 : 0;

	ui_screen_clean();
	
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, SIM_TEST_CASE);

	memset(sim1_buf, 0, sizeof(sim1_buf));
	memset(sim2_buf, 0, sizeof(sim2_buf));
	if (sim1_in == 1) {
		gr_color(0, 255, 0, 255);
		sprintf(sim1_buf, "SIM1: %s", "passed");
	} else {
		gr_color(255, 0, 0, 255);
		sprintf(sim1_buf, "SIM1: %s", "failed");
	}
	ui_draw_title(gr_fb_height() / 2, sim1_buf);

	if (sim2_in == 1) {
		gr_color(0, 255, 0, 255);
		sprintf(sim2_buf, "SIM2: %s", "passed");
	} else {
		gr_color(255, 0, 0, 255);
		sprintf(sim2_buf, "SIM2: %s", "failed");
	}
	ui_draw_title(gr_fb_height() / 2 + 2 * CHAR_HEIGHT, sim2_buf);

	ui_quit_with_prompt(thiz->name, thiz->passed);
	return 0;
}

static void sim_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* sim_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase));

	if (thiz != NULL) {
		pthread_t t;
		thiz->run = sim_test_run;
		thiz->destroy = sim_test_destroy;

		thiz->name = SIM_TEST_CASE;
		thiz->passed = passed;
		pthread_create(&t, NULL, sim_test_poweron_thread_run, (void*)thiz);
		//simtest_poweron();
	}
	return thiz;
}
