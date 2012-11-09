#include "led_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <string.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>


typedef struct {
	CmdListener* listener;
} PrivInfo;

static int led_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);

	char content[4];

	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_LED);

	memset(content, 0, sizeof(content));
	*(uint8_t*)content = AUTOTEST_RET_PASS;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	return 0;
}

static void led_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* led_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = led_cmd_execute;
		thiz->destroy = led_cmd_destroy;

		thiz->cmd = AUTOTEST_LED;
		priv->listener = listener;
	}

	return thiz;
}
