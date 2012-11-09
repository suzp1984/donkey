#include "pl_sensor_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define SPRD_PSENSOR_CTL            "/sys/class/p_sensor/p_sensor_class/prox_on"
#define SPRD_LSENSOR_CTL            "/sys/class/l_sensor/l_sensor_class/als_on"
#define TRACE_MSG "PL_SENSOR"

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int pl_sensor_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	int l_fd;
	int p_fd;
	char content[4];

	memset(content, 0, sizeof(content));
	l_fd = open(SPRD_LSENSOR_CTL, O_RDWR);
	p_fd = open(SPRD_PSENSOR_CTL, O_RDWR);

	if (l_fd >= 0 && p_fd >=0) {
		*(uint8_t*)content = AUTOTEST_RET_PASS;
	} else {
		*(uint8_t*)content = AUTOTEST_RET_FAIL;
	}

	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);

	/* send AUTOTEST_TRACE fail
	parcel_set_sub_cmd(reply, AUTOTEST_TRACE);
	parcel_set_content(reply, TRACE_MSG, strlen(TRACE_MSG));
	cmd_listener_reply(priv->listener, reply);
	*/

	parcel_set_sub_cmd(reply, AUTOTEST_P_L_SENSOR);
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	if (l_fd >= 0) {
		close(l_fd);
	}

	if (p_fd >= 0) {
		close(p_fd);
	}
	parcel_destroy(reply);
	return 0;
}

static void pl_sensor_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* pl_sensor_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = pl_sensor_cmd_execute;
		thiz->destroy = pl_sensor_cmd_destroy;

		thiz->cmd = AUTOTEST_P_L_SENSOR;
		priv->listener = listener;
	}

	return thiz;
}
