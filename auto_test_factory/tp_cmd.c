#include "tp_cmd.h"
#include "cmd_common.h"
#include "parcel.h"
#include "utils.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define SPRD_TS_INPUT_DEV    "ctp"

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int tp_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	char content[4];
	int fd;

	fd = open_input(SPRD_TS_INPUT_DEV, O_RDONLY);
	Parcel* reply = parcel_create();

	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_TP);

	memset(content, 0, sizeof(content));
	if (fd > 0) {
		*(uint8_t*)(content) = AUTOTEST_RET_PASS;
		close(fd);
	} else {
		*(uint8_t*)(content) = AUTOTEST_RET_FAIL;
	}

	parcel_set_content(reply, content, 4);

	cmd_listener_reply(priv->listener, reply);
	parcel_destroy(reply);

	return 0;
}

static void tp_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* tp_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = tp_cmd_execute;
		thiz->destroy = tp_cmd_destroy;

		thiz->cmd = AUTOTEST_TP;
		priv->listener = listener;
	}

	return thiz;
}
