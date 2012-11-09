#include "vibrator_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <string.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define VIBRATOR_ENABLE_DEV         "/sys/class/timed_output/vibrator/enable"

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int vibrator_on(int on_time)
{
	int fd;
	int ret;
	char buffer[8];

	fd = open(VIBRATOR_ENABLE_DEV, O_RDWR);
	if (fd < 0) {
		LOGE("%s: can not open %s", __func__, VIBRATOR_ENABLE_DEV);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", on_time);
	ret = write(fd, buffer, strlen(buffer));

	close(fd);

	return 0;
}

static int vibrator_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	char content[4];

	Parcel* reply = parcel_create();

	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_VIBRATOR);

	memset(content, 0, sizeof(content));
	// step 1: close vibrator, wait 
	*(uint8_t*)(content) = AUTOTEST_RET_STEP1;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);
	usleep(100000);

	// step 2:
	vibrator_on(100);
	*(uint8_t*)(content) = AUTOTEST_RET_STEP2;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);
	usleep(100000);

	// step 3:
	*(uint8_t*)(content) = AUTOTEST_RET_DONE;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	return 0;
}

static void vibrator_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* vibrator_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = vibrator_cmd_execute;
		thiz->destroy = vibrator_cmd_destroy;

		thiz->cmd = AUTOTEST_VIBRATOR;
		priv->listener = listener;
	}

	return thiz;
}
