#include "wifi_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

static pthread_mutex_t wifi_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
	CmdListener* listener;
	int passed;
	pthread_t t;
} PrivInfo;

static void wifi_test(int is_on)
{
	if (is_on) {
		/*
		system("echo "" > /data/misc/wifi/user_priority.conf");
		system("chown wifi.wifi /data/misc/wifi/user_priority.conf");
		if(access("/data/misc/wifi/wpa_supplicant.conf",F_OK)){
			system("cp /system/etc/wifi/wpa_supplicant.conf /data/misc/wifi/");
			system("chown wifi.wifi /data/misc/wifi/wpa_supplicant.conf");
		}
		system("rmmod unifi_sdio");
		system("synergy_service &");
		system("insmod /system/lib/modules/unifi_sdio.ko");
		sleep(3);
		system("setprop ctl.start wpa_supplicant");
		system("wpa_cli scan");
		sleep(2);
		system("wpa_cli scan_results > /data/wifi_test.txt");
		usleep(50000); */
		system("rmmod unifi_sdio");
		sleep(1);
		system("insmod /system/lib/modules/unifi_sdio.ko");
		sleep(4);
		system("iwlist wlan0 scanning > /data/wifi_test.txt");
		sleep(2);
	} else {
		system("rm -f /data/wifi_test.txt");
	}
}

static int wifi_cmd_test_start(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);
	FILE* fd;
	int len = 0;
	char buffer[1024];
	char* ptr = NULL;

	wifi_test(1);
	fd = fopen("/data/wifi_test.txt", "r");
	if (fd == NULL) {
		goto out;
	}

	do {
		ptr = fgets(buffer, 1024, fd);
		if (ptr == NULL)
			break;
		len++;
	} while(1);

	fclose(fd);
out:
	wifi_test(0);

	priv->passed = len > 5 ? 1 : 0;
	return 0;
}

static void* wifi_cmd_thread_run(void* ctx)
{
	CmdInterface* thiz = (CmdInterface*)ctx;
	DECLES_PRIV(priv, thiz);

	pthread_mutex_lock(&wifi_lock);

	wifi_cmd_test_start(thiz);

	pthread_mutex_unlock(&wifi_lock);
	return NULL;
}

static int wifi_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);

	char content[4];
	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_WIFI);

	memset(content, 0, sizeof(content));

	pthread_mutex_lock(&wifi_lock);
	if (priv->passed == 1) {
		*(uint8_t*)content = AUTOTEST_RET_PASS;
	} else {
		*(uint8_t*)content = AUTOTEST_RET_FAIL;
	}
	pthread_mutex_unlock(&wifi_lock);

	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	return 0;
}

static void wifi_cmd_destroy(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);
	pthread_join(priv->t, NULL);
	SAFE_FREE(thiz);
}

CmdInterface* wifi_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = wifi_cmd_execute;
		thiz->destroy = wifi_cmd_destroy;

		thiz->cmd = AUTOTEST_WIFI;
		priv->listener = listener;
		priv->passed = 0;
		pthread_create(&priv->t, NULL, wifi_cmd_thread_run, (void*)thiz);
	}

	return thiz;
}
