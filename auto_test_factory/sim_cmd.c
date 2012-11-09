#include "sim_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "engat.h"  
#include "engapi.h"

#define LOG_TAG "auto_test"
#include <utils/Log.h>

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int simtest_poweron(void) 
{
	int fd;
	char cmd[32];

	//open sim1
	fd = engapi_open(0);
	if (fd < 0) {
		LOGE("%s: open sim1 fail", __func__);
		return -1;
	}

	do {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd , "%d,%d,%s",ENG_AT_NOHANDLE_CMD, 1, "AT+SFUN=2");
		LOGE("%s: send %s", __func__, cmd);
		engapi_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		engapi_read(fd, cmd, sizeof(cmd));
		LOGE("%s: response=%s", __func__, cmd);
		sleep(2);
	} while(strstr(cmd, "OK") == NULL);

	do {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "%d,%d,%s", ENG_AT_NOHANDLE_CMD, 1, "AT+SFUN=4");
		LOGE("%s: send %s", __func__, cmd);
		engapi_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		engapi_read(fd, cmd, sizeof(cmd));
		LOGE("%s: response=%s", __func__, cmd);
		sleep(2);
	} while(strstr(cmd, "OK") == NULL);

	engapi_close(fd);

	// open sim2
	fd = engapi_open(1);
	if (fd < 0) {
		LOGE("%s: open sim2 fail", __func__);
		return -1;
	}

	do {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd , "%d,%d,%s",ENG_AT_NOHANDLE_CMD, 1, "AT+SFUN=2");
		LOGE("%s: send %s", __func__, cmd);
		engapi_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		engapi_read(fd, cmd, sizeof(cmd));
		LOGE("%s: response=%s", __func__, cmd);
		sleep(2);
	} while(strstr(cmd, "OK") == NULL);

	do {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "%d,%d,%s", ENG_AT_NOHANDLE_CMD, 1, "AT+SFUN=4");
		LOGE("%s: send %s", __func__, cmd);
		engapi_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		engapi_read(fd, cmd, sizeof(cmd));
		LOGE("%s: response=%s", __func__, cmd);
		sleep(2);
	} while(strstr(cmd, "OK") == NULL);

	engapi_close(fd);

	return 0;
}

static int simtest_checksim(int type) 
{
	int fd, ret = 0;
	char cmd[32];
	char simstatus;
	fd = engapi_open(type);

	if (fd < 0) {
		LOGE("%s: engapi_open fail", __func__);
		return 0;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%d,%d", ENG_AT_EUICC, 0);
	engapi_write(fd, cmd, strlen(cmd));
	memset(cmd, 0, sizeof(cmd));
	engapi_read(fd, cmd, sizeof(cmd));

	simstatus = cmd[0];
	if (simstatus == '1') {
		LOGE("%s: simstatus is %c", __func__, simstatus);
		ret = 0;
	} else if ((simstatus == '0') || (simstatus == '2')) {
		LOGE("%s: simstatus is %c", __func__, simstatus);
		ret = 1;
	}

	engapi_close(fd);

	return ret;
}

static int sim_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	int i = 0;
	int sim1_in = 0;
	int sim2_in = 0;

	for(i = 0; i < 3; i++) {
		sim1_in = simtest_checksim(0);
		sim2_in = simtest_checksim(1);
		if (sim1_in == 1 || sim2_in == 1) {
			break;
		}
		sleep(1);
	}

	LOGE("%s: sim1 status %d", __func__, sim1_in);
	LOGE("%s: sim2 status %d", __func__, sim2_in);

	char content[4];
	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_SIM);

	memset(content, 0, sizeof(content));
	if (sim1_in == 1 && sim2_in == 1) {
		*(uint8_t*)content = AUTOTEST_RET_PASS;
	} else {
		*(uint8_t*)content = AUTOTEST_RET_FAIL;
	}
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	return 0;
}

static void sim_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* sim_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = sim_cmd_execute;
		thiz->destroy = sim_cmd_destroy;

		thiz->cmd = AUTOTEST_SIM;
		priv->listener = listener;
		simtest_poweron();
	}

	return thiz;
}
