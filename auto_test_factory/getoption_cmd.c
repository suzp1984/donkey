#include "getoption_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define HWINFO_PATH "/proc/hwinfo"
#define OPTION_FLAGS "optionflags"

typedef struct {
	CmdListener* listener;
	uint16_t value[2];
} PrivInfo;

static int hexstr2dec(const char* hex)
{
	int dec = 0;
	int size = strlen(hex);
	int i;

	for (i = 0; i < size; i++) {
		int m = 0;
		char c = hex[size - i - 1];
		if ( c == 'X' || c == 'x' )
			break;
		if (c >= '0' && c <= '9') {
			m = c - '0';
		} else if (c >= 'A' && c <= 'F') {
			m = c - 'A' + 10;
		} else if (c >= 'a' && c <= 'f') {
			m = c - 'a' + 10;
		}

		if (i == 0) {
			dec += m;
		} else {
			dec += m * pow(16, i);
		}
	}

	return dec;
}

static int getoption_cmd_init(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);

	FILE* fd;
	char buffer[1024];
	char* s = NULL;
	char* ptr = NULL;
	int i = 0;

	fd = fopen(HWINFO_PATH, "r");

	if (fd == NULL) {
		LOGE("%s: can not open %s", __func__, HWINFO_PATH);
		return -1;
	}

	do {
		s = fgets(buffer, 1024, fd);
		if (s == NULL) {
			goto out;
		}
		ptr = strstr(s, OPTION_FLAGS);
		if (ptr != NULL) {
			LOGE("%s: get keymay: %s", __func__, ptr);
			break;
		}
	} while(1);

	ptr = strchr(ptr, '=');
	ptr++;

	i = 0;
	s = strtok(ptr, ",");
	while(s != NULL) {
		while(*s != '0') {
			s++;
		}
		ptr = s;
		while(*ptr != '\0') {
			ptr ++;
		}
		ptr--;
		while(isspace(*ptr)) {
			*ptr = '\0';
			ptr--;
		}

		LOGE("%s: str: %s\n", __func__, s);
		uint16_t value = (uint16_t)hexstr2dec(s);
		if (i < 2) {
			priv->value[i] = value;
		}
		i++;
		LOGE("%s: 0x%x\n", __func__, value);
		s = strtok(NULL, ",");
	}

out:
	fclose(fd);

	return 0;
}

static int getoption_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);

	char content[8];
	Parcel* reply = parcel_create();

	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_GETOPTIONFLAGS);

	memset(content, 0, sizeof(content));
	*(uint8_t*)content = AUTOTEST_RET_PASS;
	/*
	*(uint8_t*)(content + 4) = 0xff;
	*(uint8_t*)(content + 5) = 0xff;
	*(uint8_t*)(content + 6) = 0xff;
	*(uint8_t*)(content + 7) = 0xff; */
	*(uint16_t*)(content + 4) = priv->value[1];
	*(uint16_t*)(content + 6) = priv->value[0];

	parcel_set_content(reply, content, 8);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	return 0;
}

static void getoption_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* getoption_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = getoption_cmd_execute;
		thiz->destroy = getoption_cmd_destroy;

		thiz->cmd = AUTOTEST_GETOPTIONFLAGS;
		priv->listener = listener;
		priv->value[0] = 0;
		priv->value[1] = 0;
		getoption_cmd_init(thiz);
	}

	return thiz;
}
