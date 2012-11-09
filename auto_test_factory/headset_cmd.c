#include "headset_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define SPRD_HEADSET_SWITCH_DEV         "/sys/class/switch/h2w/state"

#define LOG_TAG "auto_test"
#include <utils/Log.h>

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int headset_cmd_headset_status(int fd)
{
	char buffer[8];
	int ret = 0;
	int status;

	memset(buffer, 0, sizeof(buffer));
	lseek(fd, 0, SEEK_SET);

	ret = read(fd, buffer, sizeof(buffer));
	if (ret < 0) {
		LOGE("%s: read %s fail", __func__, SPRD_HEADSET_SWITCH_DEV);
		return -1;
	}
	status = atoi(buffer);
	LOGE("%s,status =%d \n",__func__,status);
	return status;
}

static int headset_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);

	char content[4];
	int status;
	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_HEADSET);

	memset(content, 0, sizeof(content));
	int fd = open(SPRD_HEADSET_SWITCH_DEV, O_RDONLY);

	if (fd < 0) {
		*(uint8_t*)(content) = AUTOTEST_RET_FAIL;
		parcel_set_content(reply, content, 4);
		cmd_listener_reply(priv->listener, reply);
		LOGE("%s: open %s failed", __func__, SPRD_HEADSET_SWITCH_DEV);
		return -1;
	}
	usleep(150000);

	status = headset_cmd_headset_status(fd);

	if (status == 0) {
		LOGE("%s:status = %d fail\n",__func__,status);
		*(uint8_t*)(content) = AUTOTEST_RET_FAIL;
		goto out;
	}
	
	*(uint8_t*)(content) = AUTOTEST_RET_STEP1;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	usleep(150000);

	status = headset_cmd_headset_status(fd);

	if (status == 0) {
		*(uint8_t*)(content) = AUTOTEST_RET_PASS;

	} else if (status > 0) {
		*(uint8_t*)(content) = AUTOTEST_RET_FAIL;
	}

out:
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	close(fd);
	return 0;
}

static void headset_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* headset_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = headset_cmd_execute;
		thiz->destroy = headset_cmd_destroy;

		thiz->cmd = AUTOTEST_HEADSET;
		priv->listener = listener;
	}

	return thiz;
}
