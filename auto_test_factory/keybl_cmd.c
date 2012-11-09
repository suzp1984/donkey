#include "keybl_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define KEY_BACKLIGHT_DEV           "/sys/class/leds/keyboard-backlight/brightness"
#define KEY_BACKLIGHT_MAX_DEV       "/sys/class/leds/keyboard-backlight/max_brightness"

typedef struct {
	CmdListener* listener;
	 
	int key_max_bright;
} PrivInfo;

static int keybl_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	char content[4];
	char max_bright[8];
	memset(max_bright, 0, sizeof(max_bright));
	sprintf(max_bright, "%d", priv->key_max_bright);

	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_KEYBL);

	memset(content, 0, sizeof(content));

	int fd = open(KEY_BACKLIGHT_DEV, O_RDWR);
	if (fd < 0) {
		*(uint8_t*)(content) = AUTOTEST_RET_FAIL;
		parcel_set_content(reply, content, 4);
		cmd_listener_reply(priv->listener, reply);
		LOGE("%s: open %s fail", __func__, KEY_BACKLIGHT_DEV);
		goto out;
	}

	// step 1 bright key bl
	write(fd, max_bright, strlen(max_bright));

	*(uint8_t*)content = AUTOTEST_RET_STEP1;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);
	usleep(200000);

	write(fd, "0", 1);
	*(uint8_t*)content = AUTOTEST_RET_DONE;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	close(fd);
out:
	parcel_destroy(reply);

	return 0;
}

static void keybl_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* keybl_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = keybl_cmd_execute;
		thiz->destroy = keybl_cmd_destroy;

		thiz->cmd = AUTOTEST_KEYBL;
		priv->listener = listener;

		char buf[8];
		int fd = open(KEY_BACKLIGHT_MAX_DEV, O_RDONLY);
		if (fd < 0) {
			LOGE("%s: open %s fail", __func__, KEY_BACKLIGHT_MAX_DEV);
			priv->key_max_bright = 0;
		} else {
			memset(buf, 0, sizeof(buf));
			read(fd, buf, sizeof(buf));
			priv->key_max_bright = atoi(buf);
			close(fd);
		}
	}

	return thiz;
}
