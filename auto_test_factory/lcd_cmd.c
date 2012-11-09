#include "lcd_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define SPRD_LCD_AUTOTEST_PATH "/sys/devices/virtual/lcd_test/lcd/device"
#define LCD_AUTOTEST "autotest"

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int lcd_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);

	char content[4];
	int ret = -1;
	Parcel* reply = parcel_create();

	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_LCD);

	memset(content, 0, sizeof(content));
	int fd = open(SPRD_LCD_AUTOTEST_PATH, O_WRONLY);

	if (fd >= 0) {
		ret = write(fd, LCD_AUTOTEST, strlen(LCD_AUTOTEST));
		close(fd);
	} else {
		LOGE("%s: open %s fail", __func__, SPRD_LCD_AUTOTEST_PATH);
	}

	if (ret < 0) {
		*(uint8_t*)(content) = AUTOTEST_RET_FAIL;
	} else {
		*(uint8_t*)(content) = AUTOTEST_RET_PASS;
	}

	parcel_set_content(reply, content, 4);

	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	return 0;
}

static void lcd_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* lcd_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = lcd_cmd_execute;
		thiz->destroy = lcd_cmd_destroy;

		thiz->cmd = AUTOTEST_LCD;
		priv->listener = listener;
	}

	return thiz;
}
