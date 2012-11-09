#include "flashlight_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <linux/videodev2.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SPRD_DCAM_DEV               "/dev/video0"

typedef struct {
	CmdListener* listener;

	int fd;
} PrivInfo;

static int flashlight_cmd_enable(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);

	struct v4l2_control ctrl;

	ctrl.id = V4L2_CID_GAMMA;
	ctrl.value = 2;

	ioctl(priv->fd, VIDIOC_S_CTRL, &ctrl);
	return 0;
}

static void flashlight_cmd_disable(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);

	struct v4l2_control ctrl;

	ctrl.id = V4L2_CID_GAMMA;
	ctrl.value = 0;

	ioctl(priv->fd, VIDIOC_S_CTRL, &ctrl);
}

static int flashlight_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);

	char content[4];
	struct v4l2_streamparm streamparm;
	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_FLASHLIGHT);

	memset(content, 0, sizeof(content));
	memset(&streamparm, 0, sizeof(streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	streamparm.parm.capture.capturemode = 0;
	streamparm.parm.raw_data[199] = 1;
	streamparm.parm.raw_data[198] = 0;

	priv->fd = open(SPRD_DCAM_DEV, O_RDWR);
	ioctl(priv->fd, VIDIOC_S_PARM, &streamparm);

	flashlight_cmd_disable(thiz);
	*(uint8_t*)content = AUTOTEST_RET_STEP1;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);
	usleep(100000);

	flashlight_cmd_enable(thiz);
	*(uint8_t*)content = AUTOTEST_RET_STEP2;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);
	usleep(100000);

	flashlight_cmd_disable(thiz);
	*(uint8_t*)content = AUTOTEST_RET_DONE;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);

	close(priv->fd);

	return 0;
}

static void flashlight_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* flashlight_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = flashlight_cmd_execute;
		thiz->destroy = flashlight_cmd_destroy;

		thiz->cmd = AUTOTEST_FLASHLIGHT;
		priv->listener = listener;
	}

	return thiz;
}
