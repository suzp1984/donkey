#include "rtc_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int rtc_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);

	time_t time1;
	time_t time2;
	time(&time1);
	usleep(1000000);
	time(&time2);

	int ret = (int)(time2 - time1);
	LOGE("%s: time ret %d", __func__, ret);
	char content[4];

	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_RTC);

	memset(content, 0, sizeof(content));
	if (ret == 1) {
		*(uint8_t*)content = AUTOTEST_RET_PASS;
	} else {
		*(uint8_t*)content = AUTOTEST_RET_FAIL;
	}
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	return 0;
}

static void rtc_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* rtc_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = rtc_cmd_execute;
		thiz->destroy = rtc_cmd_destroy;

		thiz->cmd = AUTOTEST_RTC;
		priv->listener = listener;
	}

	return thiz;
}
