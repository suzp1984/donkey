#include "fm_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define SPRD_FM_DEV             "/dev/rda5802_fm"

#define KT0812G_FM_IOCTL_BASE     'R'
#define KT0812G_FM_IOCTL_ENABLE      _IOW(KT0812G_FM_IOCTL_BASE, 0, int)
#define KT0812G_FM_IOCTL_GET_ENABLE  _IOW(KT0812G_FM_IOCTL_BASE, 1, int)
#define KT0812g_FM_IOCTL_SET_TUNE    _IOW(KT0812G_FM_IOCTL_BASE, 2, int)
#define KT0812g_FM_IOCTL_GET_FREQ    _IOW(KT0812G_FM_IOCTL_BASE, 3, int)
#define KT0812G_FM_IOCTL_SEARCH      _IOW(KT0812G_FM_IOCTL_BASE, 4, int[4])
#define KT0812G_FM_IOCTL_STOP_SEARCH _IOW(KT0812G_FM_IOCTL_BASE, 5, int)
#define KT0812G_FM_IOCTL_MUTE        _IOW(KT0812G_FM_IOCTL_BASE, 6, int)
#define KT0812G_FM_IOCTL_SET_VOLUME  _IOW(KT0812G_FM_IOCTL_BASE, 7, int)
#define KT0812G_FM_IOCTL_GET_VOLUME  _IOW(KT0812G_FM_IOCTL_BASE, 8, int)

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int fm_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);

	int fd = open(SPRD_FM_DEV, O_RDWR);
	int on = 1;
	int default_tune = 875;
	int pass = 1;
	char content[4];
	Parcel* reply = parcel_create();

	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_FM);

	if (fd < 0) {
		pass = 0;
		LOGE("%s: fail to open %s", __func__, SPRD_FM_DEV);
		goto out;
	}

	if (ioctl(fd, KT0812G_FM_IOCTL_ENABLE, &on) < 0) {
		pass = 0;
		LOGE("%s: ioctl KT0812G_FM_IOCTL_ENABLE fail", __func__);
	}

	if (ioctl(fd, KT0812g_FM_IOCTL_SET_TUNE, &default_tune) < 0) {
		pass = 0;
		LOGE("%s: ioctl KT0812g_FM_IOCTL_SET_TUNE fail", __func__);
	}

	memset(content, 0, sizeof(content));

out:
	if (pass == 1) {
		*(uint8_t*)content = AUTOTEST_RET_PASS;
	} else {
		*(uint8_t*)content = AUTOTEST_RET_FAIL;
	}

	on = 0;
	if (ioctl(fd, KT0812G_FM_IOCTL_ENABLE, &on) < 0) {
		LOGE("%s: ioctl disble FM fail", __func__);
	}

	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);

	if (fd >= 0) {
		close(fd);
	}
	return 0;
}

static void fm_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* fm_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = fm_cmd_execute;
		thiz->destroy = fm_cmd_destroy;

		thiz->cmd = AUTOTEST_FM;
		priv->listener = listener;
	}

	return thiz;
}
