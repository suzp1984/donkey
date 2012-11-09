#include "wifi_test.h"
#include "ui.h"
#include "minui.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define WIFI_WAIT_PROMPT "wait..."

#define ADDRESS_WIFI "Address:"
#define ESSID_WIFI "ESSID:"

struct wifi_cell {
	char ssid[128];
	char address[20];
};

typedef struct {
	int passed;
} PrivInfo;

static pthread_mutex_t wifi_lock = PTHREAD_MUTEX_INITIALIZER;

static void wifi_test(int is_on);
static int wifi_test_start(TestCase* thiz);

static void wifi_test(int is_on)
{
	if (is_on) {
		LOGE("%s: rmmod unifi_sdio", __func__);
		system("rmmod unifi_sdio");
		sleep(1);
		LOGE("%s: insmod unifi_sdio", __func__);
		system("insmod /system/lib/modules/unifi_sdio.ko");
		sleep(2);
		system("iwlist wlan0 scanning > /data/wifi_test.txt");
		sleep(2);
	} else {
		system("rm -f /data/wifi_test.txt");
	}
}

static int wifi_test_start(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	LOGE("%s: start wifi test...", __func__);
	FILE* fd;
	int cell_num = 0;
	char buffer[1024];
	char* ptr = NULL;
	struct wifi_cell cell;
	int start_y = 50;
	char cell_info[148];

	wifi_test(1);

	gr_color(0, 0, 0, 255);
	ui_draw_title(gr_fb_height()/2, WIFI_WAIT_PROMPT);

	fd = fopen("/data/wifi_test.txt", "r");
	if (fd == NULL) {
		LOGE("%s: open /data/wifi_test.txt fail", __func__);
		goto out;
	}

	gr_color(0, 255, 255, 255);

	do {
		char* tmp = NULL;
		ptr = fgets(buffer, 1024, fd);
		if (ptr == NULL)
			break;

		tmp = strstr(ptr, ADDRESS_WIFI);
		if (tmp != NULL) {
			tmp += strlen(ADDRESS_WIFI) + 1;
			memset(cell.address, 0, sizeof(cell.address));
			strncpy(cell.address, tmp, strlen(tmp) - 1);
		}

		tmp = strstr(ptr, ESSID_WIFI);
		if (tmp != NULL) {
			char* m;
			tmp += strlen(ESSID_WIFI) + 1;
			m = strchr(tmp, '"');
			memset(cell.ssid, 0, sizeof(cell.ssid));
			strncpy(cell.ssid, tmp, m - tmp);
			memset(cell_info, 0, sizeof(cell_info));
			sprintf(cell_info, "%s (%s)", cell.ssid, cell.address);
			gr_text(10, start_y, cell_info);
			start_y += CHAR_HEIGHT;
			cell_num ++;
		}

	} while(1);

	fclose(fd);
out:
	wifi_test(0);

	priv->passed = cell_num > 0 ? 1 : 0;

	if (priv->passed) {
		return 0;
	} else {
		return -1;
	}
}

static void* wifi_test_thread_run(void* ctx)
{
	TestCase* thiz = (TestCase*)ctx;
	int i = 0;

	pthread_mutex_lock(&wifi_lock);

	for (i = 0; i < 3; i++) {
		if (wifi_test_start(thiz) == 0) {
			break;
		}

		LOGE("%s: wifi_test fail, try again", __func__);
	}

	pthread_mutex_unlock(&wifi_lock);
	return NULL;
}

static int wifi_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	pthread_t t;

	pthread_create(&t, NULL, wifi_test_thread_run, (void*)thiz);

	ui_screen_clean();

	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, WIFI_TEST_CASE);
	
	gr_color(255, 0, 0, 255);
	ui_draw_title(gr_fb_height()/2, WIFI_WAIT_PROMPT);
	gr_flip();

	sleep(1);
	pthread_mutex_lock(&wifi_lock);

	gr_color(0, 0, 0, 255);
	ui_draw_title(gr_fb_height()/2, WIFI_WAIT_PROMPT);

	thiz->passed = priv->passed;
	ui_quit_with_prompt(thiz->name, thiz->passed);
	pthread_mutex_unlock(&wifi_lock);

	return 0;
}

static void wifi_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* wifi_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = wifi_test_run;
		thiz->destroy = wifi_test_destroy;

		thiz->name = WIFI_TEST_CASE;
		thiz->passed = passed;
		priv->passed = 0;
	}

	return thiz;
}
