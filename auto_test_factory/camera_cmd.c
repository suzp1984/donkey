#include "camera_cmd.h"
#include "cmd_common.h"
#include "parcel.h"
#include "cam_data.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/android_pmem.h>
#include <unistd.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define SPRD_DCAM_DISPLAY_WIDTH     640
#define SPRD_DCAM_DISPLAY_HEIGHT    480

#define SPRD_DCAM_DEV               "/dev/video0"
#define SPRD_MAX_PREVIEW_BUF        2
#define SPRD_PMEM_DEV               "/dev/pmem_adsp"

#define CAM_PMEM_ERROR_MSG "camera_cmd_pmem_init fail"

struct cam_buf {
	uint32_t phys_addr;
	uint32_t virt_addr;
	unsigned int length;
};

typedef struct {
	CmdListener* listener;
	
	int v4l2_fd;
	int pmem_fd;
	int passed;
	struct pmem_region sprd_pmem;
	void* pmem_base;
	int offset_page_align;
	struct cam_buf buffers[2];
} PrivInfo;

static int camera_cmd_pmem_init(CmdInterface* thiz, int size)
{
	DECLES_PRIV(priv, thiz);

	priv->pmem_fd = open(SPRD_PMEM_DEV, O_RDWR, 0);

	if (priv->pmem_fd >= 0) {
		if (ioctl(priv->pmem_fd, PMEM_GET_TOTAL_SIZE, &priv->sprd_pmem) < 0) {
			LOGE("%s: ioctl PMEM_GET_TOTAL_SIZE fail", __func__);
			return -1;
		}

		priv->pmem_base = mmap(0, size, PROT_READ | PROT_WRITE,
				MAP_SHARED, priv->pmem_fd, 0);
		if (MAP_FAILED == priv->pmem_base) {
			LOGE("%s: pmem mmap failed", __func__);
			close(priv->pmem_fd);
			return -1;
		}

		priv->sprd_pmem.len = size;
		if (ioctl(priv->pmem_fd, PMEM_GET_PHYS, &priv->sprd_pmem) < 0) {
			LOGE("%s: PMEM_GET_PHYS fail", __func__);
			return -1;
		}
	} else {
		return -1;
	}

	int buffer_size = SPRD_DCAM_DISPLAY_WIDTH*SPRD_DCAM_DISPLAY_HEIGHT*2;
	int page_size = getpagesize();
	priv->offset_page_align = (buffer_size + page_size - 1) & ~(page_size - 1);

	memset(priv->buffers, 0, sizeof(priv->buffers));
	priv->buffers[0].virt_addr = (uint32_t)priv->pmem_base;
	priv->buffers[0].phys_addr = priv->sprd_pmem.offset;
	priv->buffers[0].length = priv->offset_page_align;

	priv->buffers[1].virt_addr = (uint32_t)((uint32_t)priv->pmem_base + priv->offset_page_align);
	priv->buffers[1].phys_addr = priv->sprd_pmem.offset + priv->offset_page_align;
	priv->buffers[1].length = priv->offset_page_align;

	return 0;
}

static int camera_cmd_pmem_close(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);

	if (priv->pmem_base != NULL) {
		munmap(priv->pmem_base, SPRD_DCAM_DISPLAY_WIDTH*SPRD_DCAM_DISPLAY_HEIGHT*2*4);
	}

	if (priv->pmem_fd >= 0) {
		close(priv->pmem_fd);
	}
	return 0;
}

static int camera_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	char content[4];
	int i = 0;
	char* buf_cam = NULL;
	LOGE("%s: ********** camera test *************", __func__);

	struct v4l2_streamparm streamparm;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_crop crop;
	struct v4l2_cropcap cropcap;
	struct v4l2_capability cap;
	enum v4l2_buf_type type;
	struct v4l2_buffer buf;
	struct v4l2_control ctrl;

	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_CAMERA);

	if (camera_cmd_pmem_init(thiz, SPRD_DCAM_DISPLAY_WIDTH*SPRD_DCAM_DISPLAY_HEIGHT*2*4) < 0) {
		LOGE("%s: camera_cmd_pmem_init fail", __func__);
		cmd_listener_send_trace(priv->listener, CAM_PMEM_ERROR_MSG);
		priv->passed = 0;
		goto out;
	}

	priv->v4l2_fd = open(SPRD_DCAM_DEV, O_RDWR);
	if (priv->v4l2_fd < 0) {
		LOGE("%s: open %s fail", __func__, SPRD_DCAM_DEV);
		priv->passed = 0;
		goto out;
	}

	/* VIDIOC_S_PARM */
	memset(&streamparm, 0, sizeof(streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	streamparm.parm.capture.capturemode = 0;
	streamparm.parm.raw_data[199] = 1;
	streamparm.parm.raw_data[198] = 0;

	if (ioctl(priv->v4l2_fd, VIDIOC_S_PARM, &streamparm) < 0) {
		LOGE("%s: ioctl VIDIOC_S_PARM fail", __func__);
		priv->passed = 0;
		goto out;
	}

	/* VIDIOC_S_FMT */
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = SPRD_DCAM_DISPLAY_WIDTH;
	fmt.fmt.pix.height = SPRD_DCAM_DISPLAY_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	
	if (ioctl(priv->v4l2_fd, VIDIOC_S_FMT, &fmt) < 0) {
		LOGE("%s: ioctl VIDIOC_S_FMT fail", __func__);
		priv->passed = 0;
		goto out;
	}

	/* VIDIOC_CROPCAP */
	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(priv->v4l2_fd, VIDIOC_CROPCAP, &cropcap) == 0) {
		memset(&crop, 0, sizeof(crop));
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c.left = 0;
		crop.c.top = 0;
		crop.c.width = SPRD_DCAM_DISPLAY_WIDTH;
		crop.c.height = SPRD_DCAM_DISPLAY_HEIGHT;

		if (ioctl(priv->v4l2_fd, VIDIOC_S_CROP, &crop) < 0) {
			priv->passed = 0;
			LOGE("%s: ioctl VIDIOC_S_CROP fail", __func__);
			goto out;
		}
	} else {
		priv->passed = 0;
		goto out;
	}

	/* VIDIOC_S_CTRL */
	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.id = V4L2_CID_COLOR_KILLER;
	ctrl.value = 0;
	if (ioctl(priv->v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0) {
		LOGE("%s: ioctl VIDIOC_S_CTRL 2 fail", __func__);
		priv->passed = 0;
		goto out;
	}

	/* VIDIOC_REQBUFS */
	memset(&req, 0, sizeof(req));
	req.count = SPRD_MAX_PREVIEW_BUF;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (ioctl(priv->v4l2_fd, VIDIOC_REQBUFS, &req) < 0) {
		LOGE("%s: ioctl VIDIOC_REQBUFS fail", __func__);
		priv->passed = 0;
		goto out;
	}

	/* VIDIOC_QBUF */
	for (i = 0; i < SPRD_MAX_PREVIEW_BUF; i++) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.m.userptr = priv->buffers[i].phys_addr;
		buf.length = priv->buffers[i].length;

		if (ioctl(priv->v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
			LOGE("%s: ioctl VIDIOC_QBUF fail", __func__);
			priv->passed = 0;
			goto out;
		}
	}

	/* VIDIOC_STREAMON */
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(priv->v4l2_fd, VIDIOC_STREAMON, &type) < 0) {
		LOGE("%s: ioctl VIDIOC_STREAMON fail", __func__);
		priv->passed = 0;
		goto out;
	}

	//usleep(100000);
	buf_cam = (char*)malloc(priv->buffers[0].length);
	memset(buf_cam, 0, priv->buffers[0].length);

	for (i = 0; i < 3; i++) {
		/* get one buf */
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (ioctl(priv->v4l2_fd, VIDIOC_DQBUF, &buf) < 0) {
			LOGE("%s: ioctl VIDIOC_DQBUF fail", __func__);
			priv->passed = 0;
			goto out;
		}

		//LOGE("%s: get buf index is %d", __func__, buf.index);
		if (i == 2) {
			LOGE("%s: get %drd frame", __func__, i);
			memcpy(buf_cam, (void*)(priv->buffers[buf.index].virt_addr), priv->buffers[buf.index].length);
		}

		if (ioctl(priv->v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
			LOGE("%s: qbuf fail", __func__);
			priv->passed = 0;
			goto out;
		}
		usleep(100000);
	}

	priv->passed = 1;
	LOGE("%s: sizeof of cam_data is %d", __func__, sizeof(cam_data));
	for (i = 0; i < 460800; i++) {
		if ((uint8_t)(buf_cam[i]) != (uint8_t)(cam_data[i])) {
			LOGE("%s: %d rd bytes not equal", __func__, i);
			priv->passed = 0;
			break;
		}
	}

	/* VIDIOC_STREAMOFF */
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(priv->v4l2_fd, VIDIOC_STREAMOFF, &type) < 0) {
		LOGE("%s: VIDIOC_STREAMOFF fail", __func__);
		priv->passed = 0;
		goto out;
	}

out:
	memset(content, 0, sizeof(content));
	if (priv->passed == 1) {
		*(uint8_t*)content = AUTOTEST_RET_PASS;
	} else {
		*(uint8_t*)content = AUTOTEST_RET_FAIL;
	}

	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	if (priv->v4l2_fd >= 0) {
		close(priv->v4l2_fd);
	}

	SAFE_FREE(buf_cam);
	parcel_destroy(reply);
	camera_cmd_pmem_close(thiz);

	return 0;
}

static void camera_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* camera_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = camera_cmd_execute;
		thiz->destroy = camera_cmd_destroy;

		thiz->cmd = AUTOTEST_CAMERA;
		priv->listener = listener;
		priv->v4l2_fd  = -1;
		priv->pmem_fd = -1;
		priv->passed = 0;
		priv->pmem_base = NULL;
		priv->offset_page_align = 0;
	}

	return thiz;
}
