#include "backlight_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define LCD_BACKLIGHT_DEV           "/sys/class/leds/lcd-backlight/brightness"
#define LCD_BACKLIGHT_MAX_DEV       "/sys/class/leds/lcd-backlight/max_brightness"

typedef struct {
	CmdListener* listener;

	int lcd_max_bright;
} PrivInfo;

static int backlight_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	char content[4];
	char max_bright[8];
	memset(max_bright, 0, sizeof(max_bright));
	sprintf(max_bright, "%d", priv->lcd_max_bright);

	memset(content, 0, sizeof(content));
	Parcel* reply = parcel_create();

	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_BACKLIGHT);

	int fd = open(LCD_BACKLIGHT_DEV, O_RDWR);
	if (fd < 0) {
		*(uint8_t*)(content) = AUTOTEST_RET_FAIL;
		parcel_set_content(reply, content, 4);
		cmd_listener_reply(priv->listener, reply);
		LOGE("%s: open %s fail", __func__, LCD_BACKLIGHT_DEV);
		return -1;
	}
	//step 1: close backlight
	write(fd, "0", 1);
	*(uint8_t*)(content) = AUTOTEST_RET_STEP1;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);
	usleep(100000);

	//step 2: bright back light
	write(fd, max_bright, strlen(max_bright));
	LOGE("%s: write max_bright is %s", __func__, max_bright);
	*(uint8_t*)(content) = AUTOTEST_RET_STEP2;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);
	usleep(100000);

	//step 3: close backlight
	write(fd, "0", 1);
	*(uint8_t*)(content) = AUTOTEST_RET_DONE;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	close(fd);
	parcel_destroy(reply);
	
	return 0;
}

static void backlight_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* backlight_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		char buf[8];
		thiz->execute = backlight_cmd_execute;
		thiz->destroy = backlight_cmd_destroy;

		thiz->cmd = AUTOTEST_BACKLIGHT;

		priv->listener = listener;

		int fd = open(LCD_BACKLIGHT_MAX_DEV, O_RDONLY);
		if (fd < 0) {
			LOGE("%s: open %s fail", __func__, LCD_BACKLIGHT_MAX_DEV);
			priv->lcd_max_bright = 0;
		} else {
			memset(buf, 0, sizeof(buf));
			read(fd, buf, sizeof(buf));
			priv->lcd_max_bright = atoi(buf);
			close(fd);
		}
	}

	return thiz;
}
