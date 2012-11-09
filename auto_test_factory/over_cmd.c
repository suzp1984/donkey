#include "over_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define SIGN_AUTOTEST_FILE "/data/autotest.tmp"

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int over_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	char content[4];
	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_OVER);

	memset(content, 0, sizeof(content));
	if (!access(SIGN_AUTOTEST_FILE, F_OK)) {
		*(uint8_t*)content = AUTOTEST_RET_DONE;
	} else {
		*(uint8_t*)content = AUTOTEST_RET_PASS;
		creat(SIGN_AUTOTEST_FILE, S_IRWXU | S_IRWXG);	
	}

	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	return 0;
}

static void over_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* over_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = over_cmd_execute;
		thiz->destroy = over_cmd_destroy;

		thiz->cmd = AUTOTEST_OVER;
		priv->listener = listener;
	}

	return thiz;
}
