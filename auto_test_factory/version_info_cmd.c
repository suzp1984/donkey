#include "version_info_cmd.h"
#include "typedef.h"
#include "cmd_common.h"

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#include <cutils/properties.h>
#include <string.h>
#include "parcel.h"

#define PROP_DISPLAY_ID "ro.build.display.id"
#define DEFAULT_VERSOIN "PZ186"

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int version_info_execute(CmdInterface* thiz, void* ctx)
{
	LOGE("%s: run version info cmd", __func__);
	DECLES_PRIV(priv, thiz);
	char display_id[64];
	char* content = NULL;
	Parcel* reply = parcel_create();

	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_VERSIONINFO);

	memset(display_id, 0, sizeof(display_id));
	property_get(PROP_DISPLAY_ID, display_id, "");
	//memcpy(display_id, DEFAULT_VERSOIN, strlen(DEFAULT_VERSOIN));

	char* ptr = display_id;
	while(*ptr != '\0' && *ptr != '.') {
		ptr++;
	}

	if (*ptr == '.') {
		*ptr = '\0';
	}

	content = (char*)malloc(strlen(display_id) + 4);
	memset(content, 0, strlen(display_id) + 4);
	memcpy(content + 4, display_id, strlen(display_id));
	*(int*)(content) = AUTOTEST_RET_PASS;

	parcel_set_content(reply, content, strlen(display_id) + 4);
	cmd_listener_reply(priv->listener, reply);

	free(content);
	parcel_destroy(reply);

	return 0;
}

static void version_info_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
	return;
}

CmdInterface* version_info_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = version_info_execute;
		thiz->destroy = version_info_destroy;

		thiz->cmd = AUTOTEST_VERSIONINFO;
		priv->listener = listener;
	}

	return thiz;
}

