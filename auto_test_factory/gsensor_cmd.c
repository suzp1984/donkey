#include "gsensor_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define G_SENSOR_DEVICE        "/sys/class/g_sensor/g_sensor_class/device"

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int gsensor_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	int g_fd = -1;
	char content[4];

	memset(content, 0, sizeof(content));
	g_fd = open(G_SENSOR_DEVICE, O_RDWR);
	
	if (g_fd >= 0) {
		*(uint8_t*)content = AUTOTEST_RET_PASS;
		close(g_fd);
	} else {
		*(uint8_t*)content = AUTOTEST_RET_FAIL;
	}

	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_GSENSOR);
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);

	return 0;
}

static void gsensor_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* gsensor_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = gsensor_cmd_execute;
		thiz->destroy = gsensor_cmd_destroy;

		thiz->cmd = AUTOTEST_GSENSOR;
		priv->listener = listener;
	}

	return thiz;
}
