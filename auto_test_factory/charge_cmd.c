#include "charge_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define ENG_BATVOL      "/sys/class/power_supply/battery/real_time_voltage"
#define ENG_CHRVOL      "/sys/class/power_supply/battery/charger_voltage"
#define ENG_CURRENT     "/sys/class/power_supply/battery/real_time_current"
#define ENG_USBONLINE   "/sys/class/power_supply/usb/online"
#define ENG_ACONLINE    "/sys/class/power_supply/ac/online"

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int charge_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	char content[4];
	char buf[8];
	int vol;
	memset(buf, 0, sizeof(buf));

	//usleep(100000);
	int fd = open(ENG_CHRVOL, O_RDONLY);
	LOGE("%s: AUTOTEST_CHARGE test", __func__);

	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_CHARGE);

	memset(content, 0, sizeof(content));
	if (fd >= 0) {
		read(fd, buf, sizeof(buf));
		close(fd);
	}

	LOGE("%s: read %s is %s", __func__, ENG_USBONLINE, buf);
	vol = atoi(buf);
	if(vol > 1000) {
		LOGE("%s: pass AUTOTEST_CHARGE", __func__);
		*(uint8_t*)content = AUTOTEST_RET_PASS;
	} else {
		LOGE("%s: fail AUTOTEST_CHARGE", __func__);
		*(uint8_t*)content = AUTOTEST_RET_FAIL;
	}

	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	return 0;
}

static void charge_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* charge_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = charge_cmd_execute;
		thiz->destroy = charge_cmd_destroy;

		thiz->cmd = AUTOTEST_CHARGE;
		priv->listener = listener;
	}

	return thiz;
}
