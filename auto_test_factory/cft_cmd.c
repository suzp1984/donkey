#include "cft_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <string.h>
#include "engapi.h"
#include "engat.h"

#define LOG_TAG "auto_test"
#include <utils/Log.h>

typedef struct {
	CmdListener* listener;
	int passed;
} PrivInfo;

static int cft_cmd_test(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);
	priv->passed = 0;
	int fd;
	char cmd[1024];
	char buffer[128];
	char* ptr;
	char* start_ptr;
	char* end_ptr;

	fd = engapi_open(0);
	if (fd < 0) {
		return 0;
	}

	do {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "%d,%d,%d,%d,%d", ENG_AT_SGMR, 3, 0, 0, 3);
		engapi_write(fd, cmd, strlen(cmd));
		memset(cmd, 0, sizeof(cmd));
		engapi_read(fd, cmd, sizeof(cmd));
	} while(strstr(cmd, "desterr") != NULL);

	start_ptr = cmd;
	end_ptr = cmd + strlen(cmd);
	
	while(start_ptr < end_ptr) {
		ptr = start_ptr;
		while(*(uint8_t*)ptr != 0x0d && *(uint8_t*)ptr != 0x0a)
			ptr++;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, ptr - start_ptr + 1, "%s", start_ptr);
		LOGE("%s: %s", __func__, buffer);
		// 2345 89
		if (strstr(buffer, "BIT2") != NULL || strstr(buffer, "BIT3") != NULL
				|| strstr(buffer, "BIT4") != NULL || strstr(buffer, "BIT5") != NULL
				|| strstr(buffer, "BIT8") != NULL || strstr(buffer, "BIT9") != NULL) {
			if (strstr(buffer, "uncalibrated") != NULL) {
				goto out;
			}
		}

		while(*(uint8_t*)ptr == 0x0a || *(uint8_t*)ptr == 0x0d)
			ptr++;
		
		start_ptr = ptr;
	}

	priv->passed = 1;
out:
	engapi_close(fd);

	return 0;
}

static int cft_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	char content[4];
	Parcel* reply = parcel_create();

	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_CFT);

	cft_cmd_test(thiz);
	memset(content, 0, sizeof(content));
	if (priv->passed == 1) {
		*(uint8_t*)(content) = AUTOTEST_RET_PASS;
	} else {
		*(uint8_t*)(content) = AUTOTEST_RET_FAIL;
	}

	parcel_set_content(reply, content, 4);

	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);

	return 0;
}

static void cft_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* cft_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = cft_cmd_execute;
		thiz->destroy = cft_cmd_destroy;
		
		thiz->cmd = AUTOTEST_CFT;
		priv->listener = listener;
		priv->passed = 0;
	}

	return thiz;
}
