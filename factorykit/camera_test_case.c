#include "camera_test_case.h"
#include "ui.h"
#include "minui.h"

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#include <cutils/properties.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdint.h>
#include <linux/android_pmem.h>
#include <sys/mman.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#define HWROTATION_PROP "ro.sf.hwrotation"
#define FRONT_CAMERA_TEST_TITLE "Front Camera Test"
#define BACK_CAMERA_TEST_TITLE "Back Camera Test"
#define CAMERA_PROMPT "Press any key to continue"
#define CAMERA_TEST_FINISHED "Camera Test Finish"

#define SPRD_DCAM_DISPLAY_WIDTH     640
#define SPRD_DCAM_DISPLAY_HEIGHT    480

#define SPRD_DCAM_DEV               "/dev/video0"
#define SPRD_FB_DEV                 "/dev/graphics/fb0"
#define SPRD_PMEM_DEV               "/dev/pmem_adsp"

#define SPRD_MAX_PREVIEW_BUF        2
#define SPRD_TYPE                   2

#define BACK_CAM_CONFIG "/system/oem/back_camera.config"
#define CAMERAID_VALUE "cameraid-values"
#define BACK_CAMERA_VALUE "back_camera"

#define RGB565(r,g,b)       ((unsigned short)((((unsigned char)(r)>>3)|((unsigned short)(((unsigned char)(g)>>2))<<5))|(((unsigned short)((unsigned char)(b>>3)))<<11)))

struct frame_buffer_t {
	uint32_t phys_addr;
	uint32_t virt_addr;
	uint32_t length;
};

typedef struct {
	int reverse_screen;
	int only_back_camera;
	int v4l2_fd;
	int fb_fd;
	int pmem_fd;

	struct fb_fix_screeninfo fix;
	struct fb_var_screeninfo var;
	int camera_preview_run;

	struct frame_buffer_t fb_buf[SPRD_MAX_PREVIEW_BUF + 1];
	struct frame_buffer_t preview_buf[SPRD_MAX_PREVIEW_BUF + 1];
	struct pmem_region pmem_region;
	void* fb_bits;
	void* pmem_base;
	char* tmpbuf;
	int page_offset_align;
} PrivInfo;

static int camera_align_page(int size);
static void RGBRotate90_anticlockwise(uint8_t* des, uint8_t* src, int width, int height, int bits);
static void yuv420_to_rgb(TestCase* thiz, int width, int height, unsigned char* src, unsigned int* dst);
static void stretch_color(void* pdest, int ndestwidth, int ndestheight, int ndestbits, 
		void* psrc, int nsrcwidth, int nsrcheight, int nsrcbits);
static void camera_test_reverse_framebuf(TestCase* thiz, char* buf);

static int camera_init(TestCase* thiz, int front_back);
static void camera_streamoff(TestCase* thiz);
static void camera_preview(TestCase* thiz);
static void camera_close(TestCase* thiz);

static void camera_config_init(TestCase* thiz);
static void* camera_back_thread_run(void* ptr);
static void* camera_front_thread_run(void* ptr);
static int camera_fb_open(TestCase* thiz);
static int camera_test_pmem_init(TestCase* thiz);
static int camera_device_open(TestCase* thiz);
static int camera_test_flashled_enabled(TestCase* thiz);
static int camera_test_flashled_disable(TestCase* thiz);

static void camera_test_reverse_framebuf(TestCase* thiz, char* buf)
{
	DECLES_PRIV(priv, thiz);
	int pixel_size = priv->var.bits_per_pixel / 8;

	if (buf == NULL) {
		return;
	}

	if (priv->reverse_screen == 1) {
		char* tmp_buf = (char*)malloc(priv->var.yres * priv->var.xres * pixel_size);
		memcpy(tmp_buf, buf, (priv->var.yres * priv->var.xres * pixel_size));

		int i = 0;
		char* swap_line = (char*)malloc(priv->var.xres * pixel_size);
		for (i = 0; i < (int)priv->var.yres; i++) {
			void* tmp_dst = buf + (i * priv->var.xres * pixel_size);
			memcpy(tmp_dst, tmp_buf + priv->var.xres * (priv->var.yres-i-1)*pixel_size, 
					priv->var.xres * pixel_size);

			int j = 0;
			for (j = 0; j < (int)priv->var.xres * pixel_size; ) {
				int k = 0;
				for (k = 0; k < pixel_size; k++) {
					swap_line[j + k] = ((char*)tmp_dst)[priv->var.xres*pixel_size-j-pixel_size+k];
				}
				j += pixel_size;
			}
			memcpy(tmp_dst, swap_line, priv->var.xres * pixel_size);
		}
		free(swap_line);
		free(tmp_buf);
	}
}

static void camera_close(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	
	if (priv->v4l2_fd >= 0) {
		camera_test_flashled_disable(thiz);
		close(priv->v4l2_fd);
		priv->v4l2_fd = -1;
	}

	if (priv->fb_fd >= 0) {
		close(priv->fb_fd);
		priv->fb_fd = -1;
	}

	if (priv->pmem_fd >= 0) {
		close(priv->pmem_fd);
		priv->pmem_fd = -1;
	}

	SAFE_FREE(priv->tmpbuf);
}

static int camera_test_flashled_disable(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	struct v4l2_control ctrl;

	ctrl.id = V4L2_CID_GAMMA;
	ctrl.value = 0;

	if (ioctl(priv->v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0) {
		LOGE("%s: ioctl VIDIOC_S_CTRL error", __func__);
		return -1;
	}

	return 0;
}

static int camera_test_flashled_enabled(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	struct v4l2_control ctrl;

	ctrl.id = V4L2_CID_GAMMA;
	ctrl.value = 2;

	if (ioctl(priv->v4l2_fd, VIDIOC_S_CTRL, &ctrl) < 0) {
		LOGE("%s: ioctl VIDIOC_S_CTRL error", __func__);
		return -1;
	}

	return 0;
}

static int camera_device_open(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int count = 5;

	while((priv->v4l2_fd < 0) && (count > 0)) {
		priv->v4l2_fd = open(SPRD_DCAM_DEV, O_RDWR);
		if (priv->v4l2_fd < 0) {
			LOGE("%s: open %s fail (%s)", __func__, SPRD_DCAM_DEV, strerror(errno));
			usleep(10000);
		}

		count--;
	}

	if (count <= 0) {
		return -1;
	}

	return 0;

}

static int camera_test_pmem_init(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	int size = SPRD_DCAM_DISPLAY_WIDTH * SPRD_DCAM_DISPLAY_HEIGHT * SPRD_TYPE * 4;
	if (priv->pmem_fd < 0) {
		priv->pmem_fd = open(SPRD_PMEM_DEV, O_RDWR);
	}

	if (priv->pmem_fd >= 0) {
		if (ioctl(priv->pmem_fd, PMEM_GET_TOTAL_SIZE, &(priv->pmem_region)) < 0) {
			LOGE("%s: ioctl PMEM_GET_TOTAL_SIZE fail (%s)", __func__, strerror(errno));
		}

		priv->pmem_base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, priv->pmem_fd, 0);
		if (priv->pmem_base == MAP_FAILED) {
			priv->pmem_base = NULL;
			close(priv->pmem_fd);
			LOGE("%s: mmap pmem error (%s)", __func__, strerror(errno));
			return -1;
		}

		priv->pmem_region.len = size;
		if (ioctl(priv->pmem_fd, PMEM_GET_PHYS, &(priv->pmem_region)) < 0) {
			LOGE("%s: ioctl PMEM_GET_PHYS fail (%s)", __func__, strerror(errno));
			return -1;
		}
	} else {
		LOGE("%s: open %s fail(%s)", __func__, SPRD_PMEM_DEV, strerror(errno));
		return -1;
	}

	return 0;
}

static int camera_fb_open(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int i;
	void* bits;
	int offset_page_align;
	
	if (priv->fb_fd < 0) {
		priv->fb_fd = open(SPRD_FB_DEV, O_RDWR);
	}

	if (priv->fb_fd < 0) {
		LOGE("%s: cannot open %s: error (%s)", __func__, SPRD_FB_DEV, strerror(errno));
		return -1;
	}

	if (ioctl(priv->fb_fd, FBIOGET_FSCREENINFO, &(priv->fix)) < 0) {
		LOGE("%s: ioctl FBIOGET_FSCREENINFO fail (%s)", __func__, strerror(errno));
		return -1;
	}

	if (ioctl(priv->fb_fd, FBIOGET_VSCREENINFO, &(priv->var)) < 0) {
		LOGE("%s: ioctl FBIOGET_VSCREENINFO fail (%s)", __func__, strerror(errno));
		return -1;
	}
	
	LOGE("%s: fix.smem_len=%d", __func__, priv->fix.smem_len);

	priv->fb_bits = mmap(0, priv->fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED,
			priv->fb_fd, 0);

	if (priv->fb_bits == MAP_FAILED) {
		LOGE("%s: mmap fb failed(%s)", __func__, strerror(errno));
		return -1;
	}

	LOGE("%s: var.yres=%d, var.xres=%d", __func__, priv->var.yres, priv->var.xres);
	priv->tmpbuf = (char*)malloc(priv->var.yres * priv->var.xres * 4);
	memset(priv->tmpbuf, 0, priv->var.yres * priv->var.xres * 4);

	memset(&(priv->pmem_region), 0, sizeof(priv->pmem_region));
	camera_test_pmem_init(thiz);

	offset_page_align = camera_align_page(SPRD_DCAM_DISPLAY_WIDTH * SPRD_DCAM_DISPLAY_HEIGHT * SPRD_TYPE);
	memset(priv->preview_buf, 0, sizeof(priv->preview_buf));
	priv->preview_buf[0].virt_addr = (uint32_t)priv->pmem_base;
	priv->preview_buf[0].phys_addr = priv->pmem_region.offset;
	priv->preview_buf[0].length = offset_page_align;

	priv->preview_buf[1].virt_addr = (uint32_t)(priv->pmem_base + offset_page_align);
	priv->preview_buf[1].phys_addr = priv->pmem_region.offset + offset_page_align;
	priv->preview_buf[1].length = offset_page_align;

	priv->preview_buf[2].virt_addr = (uint32_t)(priv->pmem_base + offset_page_align * 2);
	priv->preview_buf[2].phys_addr = priv->pmem_region.offset + offset_page_align * 2;
	priv->preview_buf[2].length = offset_page_align;

	// set framebuffer address
	memset(priv->fb_buf, 0, sizeof(priv->fb_buf));
	priv->fb_buf[0].virt_addr = (uint32_t)(priv->fb_bits);
	priv->fb_buf[0].phys_addr = priv->fix.smem_start;
	priv->fb_buf[0].length = priv->var.yres * priv->var.xres * (priv->var.bits_per_pixel / 8);

	priv->fb_buf[1].virt_addr = (uint32_t)(priv->fb_bits + priv->var.yres * priv->var.xres * (priv->var.bits_per_pixel / 8));
	priv->fb_buf[1].phys_addr = priv->fix.smem_start + priv->var.yres * priv->var.xres * (priv->var.bits_per_pixel / 8);
	priv->fb_buf[1].length = priv->var.yres * priv->var.xres * (priv->var.bits_per_pixel / 8);

	priv->fb_buf[2].virt_addr = (uint32_t)priv->tmpbuf;
	priv->fb_buf[2].length = priv->var.yres * priv->var.xres * (priv->var.bits_per_pixel / 8);

	return 0;
}

static void* camera_front_thread_run(void* ptr)
{
	TestCase* thiz = (TestCase*)ptr;
	DECLES_PRIV(priv, thiz);
	
	camera_fb_open(thiz);
	camera_device_open(thiz);
	camera_init(thiz, 1);
	camera_test_flashled_enabled(thiz);

	camera_preview(thiz);

	camera_streamoff(thiz);
	camera_close(thiz);

	return NULL;
}

static void* camera_back_thread_run(void* ptr)
{
	TestCase* thiz = (TestCase*)ptr;
	DECLES_PRIV(priv, thiz);

	camera_fb_open(thiz);
	camera_device_open(thiz);
	camera_init(thiz, 0);
	camera_test_flashled_enabled(thiz);

	camera_preview(thiz);

	camera_streamoff(thiz);
	camera_close(thiz);

	return NULL;
}

static void camera_config_init(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	FILE* fd;
	char buffer[128];
	char* s = NULL;
	char* ptr = NULL;

	fd = fopen(BACK_CAM_CONFIG, "r");
	if (fd == NULL) {
		return;
	}

	do {
		s = fgets(buffer, sizeof(buffer), fd);
		if (s == NULL) {
			goto out;
		}

		while (isspace(*s))
			s++;

		if (*s == '#')
			continue;

		ptr = strstr(s, CAMERAID_VALUE);
		if (ptr != NULL) {
			break;
		}
	} while(1);

	if (ptr != NULL) {
		ptr ++;
		if (strstr(ptr, BACK_CAMERA_VALUE) != NULL) {
			priv->only_back_camera = 1;
		}
	}

out:
	fclose(fd);
}

static void RGBRotate90_anticlockwise(uint8_t* des, uint8_t* src, int width, int height, int bits)
{
	if ((!des) || (!src)) {
		return;
	}

	int n = 0;
	int linesize;
	int i, j;
	int m = bits/8;

	linesize = width * m;

	for (j = 0; j < width; j++) {
		for (i = height; i > 0; i--) {
			memcpy(&des[n], &src[linesize*(i-1)+j*m], m);
			n+=m;
		}
	}
}

static void yuv420_to_rgb(TestCase* thiz, int width, int height, unsigned char* src, unsigned int* dst)
{
	DECLES_PRIV(priv, thiz);

	int framesize = width * height;
	int j = 0; 
	int yp = 0;
	int i = 0;

	unsigned short *dst16 = (unsigned short *)dst;
	unsigned short tmp = dst16[0];
	//LOGE("%s: dst16[0] = %d", __func__, tmp);
	unsigned char *yuv420sp = src;
	for (j = 0, yp = 0; j < height; j++) {
		//LOGE("%s: j = %d", __func__, j);
		int uvp = framesize + (j >> 1) * width, u = 0, v = 0;
		for (i = 0; i < width; i++, yp++) {
			//LOGE("%s: yp = %d, uvp = %d", __func__, yp, uvp);
			int y = (0xff & ((int) yuv420sp[yp])) - 16;
			if (y < 0) y = 0;
			if ((i & 1) == 0) {
				//LOGE("%s: before set v, uvp = %d", __func__, uvp);
				v = (0xff & yuv420sp[uvp++]) - 128;
				u = (0xff & yuv420sp[uvp++]) - 128;
				//LOGE("%s: after set v, uvp = %d", __func__, uvp);
			}

			int y1192 = 1192 * y;
			int r = (y1192 + 1634 * v);
			int g = (y1192 - 833 * v - 400 * u);
			int b = (y1192 + 2066 * u);

			if (r < 0) r = 0; else if (r > 262143) r = 262143;
			if (g < 0) g = 0; else if (g > 262143) g = 262143;
			if (b < 0) b = 0; else if (b > 262143) b = 262143;

			if(priv->var.bits_per_pixel == 32) {
				//LOGE("%s: before set dst, yp = %d", __func__, yp);
				dst[yp] = ((((r << 6) & 0xff0000)>>16)<<16)|(((((g >> 2) & 0xff00)>>8))<<8)|((((b >> 10) & 0xff))<<0);
				//LOGE("%s: before set dst", __func__);
			} else {
				//LOGE("%s: before RGB565, yp = %d", __func__, yp);
				dst16[yp] = RGB565((((r << 6) & 0xff0000)>>16), (((g >> 2) & 0xff00)>>8), (((b >> 10) & 0xff)));
				//LOGE("%s: after RGB565", __func__);
			}
		}
	}
}

static void stretch_color(void* pdest, int ndestwidth, int ndestheight, int ndestbits, 
		void* psrc, int nsrcwidth, int nsrcheight, int nsrcbits)
{
	double dfAmplificationX = ((double)ndestwidth)/nsrcwidth;
	double dfAmplificationY = ((double)ndestheight)/nsrcheight;

	const int nsrccolorlen = nsrcbits / 8;
	const int ndestcolorlen = ndestbits / 8;
	int i = 0;
	int j = 0;

	for (i = 0; i < ndestheight; i++) {
		for (j = 0; j < ndestwidth; j++) {
			double tmp = i / dfAmplificationY;
			int nline = (int)tmp;

			if (tmp - nline > 0.5)
				nline ++;

			if(nline >= nsrcheight)
				nline--;

			tmp = j / dfAmplificationX;
			int nrow = tmp;

			if ((tmp - nrow) > 0.5)
				nrow++;
			if (nrow >= nsrcwidth)
				nrow--;

			unsigned char* psrcpos = (unsigned char*)psrc + (nline*nsrcwidth + nrow)*nsrccolorlen;
			unsigned char* pdestpos = (unsigned char*)pdest + (i*ndestwidth + j)*ndestcolorlen;

			*pdestpos++ = *psrcpos++;
			*pdestpos++ = *psrcpos++;
			*pdestpos++ = *psrcpos++;
			
			if (ndestcolorlen == 4)
				*pdestpos = 0;
		}
	}
}

static int camera_align_page(int size)
{
	int buffer_size, page_size;
	page_size = getpagesize();
	buffer_size = size;
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);
	LOGE("DCAM: page_size=%d; buffer_size=%d\n", page_size, buffer_size);
	return buffer_size;
}

static int camera_init(TestCase* thiz, int front_back) {
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_streamparm streamparm;
	struct v4l2_requestbuffers req;
	enum v4l2_buf_type type;

	DECLES_PRIV(priv,thiz);
	int i;

	// VIDIOC_S_PARM 
	memset(&streamparm, 0, sizeof(streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	streamparm.parm.capture.capturemode = 0;

	if (front_back == 0) { // back
		streamparm.parm.raw_data[199] = 1;
		streamparm.parm.raw_data[198] = 0;
	} else {
		streamparm.parm.raw_data[199] = 1;
		streamparm.parm.raw_data[198] = 1;
	}

	if (ioctl(priv->v4l2_fd, VIDIOC_S_PARM, &streamparm) < 0) {
		LOGE("%s: ioctl fail VIDIOC_S_PARM", __func__);
		return -1;
	}

	// VIDIOC_S_FMT
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = SPRD_DCAM_DISPLAY_WIDTH;
	fmt.fmt.pix.height = SPRD_DCAM_DISPLAY_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (ioctl(priv->v4l2_fd, VIDIOC_S_FMT, &fmt) < 0) {
		LOGE("%s: ioctl fail VIDIOC_S_FMT", __func__);
		return -1;
	}

	// VIDIOC_REQBUFS
	memset(&req, 0, sizeof(req));
	req.count = SPRD_MAX_PREVIEW_BUF;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (ioctl(priv->v4l2_fd, VIDIOC_REQBUFS, &req) < 0) {
		LOGE("%s:  ioctl fail VIDIOC_REQBUFS", __func__);
		return -1;
	}

	// VIDIOC_CROPCAP
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
			LOGE("%s: crop to (%d %d %d %d) failed", __func__, 
					crop.c.left, crop.c.top, crop.c.width, crop.c.height);
		}
	} else {
		LOGE("%s: camera has no ability to crop", __func__);
	}

	// VIDIOC_QBUF
	for (i = 0; i < SPRD_MAX_PREVIEW_BUF; i++) {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.m.userptr = priv->preview_buf[i].phys_addr;
		buf.length = priv->preview_buf[i].length;

		if (ioctl(priv->v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
			LOGE("%s: ioctl VIDIOC_QBUF fail", __func__);
			return -1;
		}
	}

	// steam on
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(priv->v4l2_fd, VIDIOC_STREAMON, &type) < 0) {
		LOGE("%s: ioctl VIDIOC_STREAMON failed", __func__);
		return -1;
	}

	return 0;
}

static void camera_streamoff(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	int i = 0;
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	for (i = 0; i < 5; i++) {
		if (ioctl(priv->v4l2_fd, VIDIOC_STREAMOFF, &type) == 0) {
			return;
		}
	}

	return;
}

static void camera_preview(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	struct v4l2_buffer buf;
	int i = 0;
	int index = 0;

	while(priv->camera_preview_run == 1) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		if (ioctl(priv->v4l2_fd, VIDIOC_DQBUF, &buf) < 0) {
			return;
		}

		for (i = 0; i < SPRD_MAX_PREVIEW_BUF; i++) {
			if (buf.m.userptr == priv->preview_buf[i].phys_addr) {
				index = i;
				break;
			}
		}

		usleep(100000);
		//LOGE("%s: before yuv420_to_rgb, index is %d", __func__, index);
		yuv420_to_rgb(thiz, SPRD_DCAM_DISPLAY_WIDTH, SPRD_DCAM_DISPLAY_HEIGHT, (unsigned char*)priv->preview_buf[index].virt_addr,
				(unsigned int*)priv->preview_buf[2].virt_addr);

		//LOGE("%s: before stretch_color", __func__);
		stretch_color((void*)priv->fb_buf[2].virt_addr, priv->var.yres, priv->var.xres, priv->var.bits_per_pixel,
				(void*)priv->preview_buf[2].virt_addr, SPRD_DCAM_DISPLAY_WIDTH, SPRD_DCAM_DISPLAY_HEIGHT, 
				priv->var.bits_per_pixel);

		//LOGE("%s: before RGBRotate90_anticlockwise", __func__);
		RGBRotate90_anticlockwise((uint8_t*)priv->fb_buf[index].virt_addr, (uint8_t*)priv->fb_buf[2].virt_addr,
				priv->var.yres, priv->var.xres, priv->var.bits_per_pixel);

		// reverse framebuffer
		camera_test_reverse_framebuf(thiz, (char*)(priv->fb_buf[index].virt_addr));

		priv->var.yres_virtual = priv->var.yres * 2;
		priv->var.yoffset = index * priv->var.yres;

		if (ioctl(priv->fb_fd, FBIOPUT_VSCREENINFO, &priv->var) < 0) {
			LOGE("%s: ioctl FBIOPUT_VSCREENINFO error", __func__);
		}

		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = index;
		buf.m.userptr = priv->preview_buf[index].phys_addr;
		buf.length = priv->preview_buf[index].length;
		if (ioctl(priv->v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
			LOGE("%s: VIDIOC_QBUF fail", __func__);
			return;
		}
	}

	munmap(priv->pmem_base, SPRD_DCAM_DISPLAY_WIDTH * SPRD_DCAM_DISPLAY_HEIGHT * SPRD_TYPE * 4);
	munmap((void*)priv->fb_buf[0].virt_addr, priv->fix.smem_len);
}

static int camera_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	pthread_t t;
	int ret = 0;

	camera_config_init(thiz);
	ui_screen_clean();

	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, FRONT_CAMERA_TEST_TITLE);
	gr_color(0, 255, 0, 255);
	ui_draw_prompt(CAMERA_PROMPT);

	priv->camera_preview_run = 1;
	pthread_create(&t, NULL, (void*)camera_front_thread_run, (void*)thiz);

	ui_wait_anykey();
	LOGE("%s: front camera test end", __func__);
	priv->camera_preview_run = 0;
	pthread_join(t, NULL);

	/*
	usleep(10000);
	ui_screen_clean();
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, BACK_CAMERA_TEST_TITLE);
	gr_color(0, 255, 0, 255);
	ui_draw_prompt(CAMERA_PROMPT);
	*/

	priv->camera_preview_run = 1;
	pthread_create(&t, NULL, (void*)camera_back_thread_run, (void*)thiz);

	ui_wait_anykey();
	priv->camera_preview_run = 0;
	pthread_join(t, NULL);

	ui_screen_clean();
	gr_flip();
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, BACK_CAMERA_TEST_TITLE);
	gr_color(0, 255, 0, 255);
	ui_draw_title(gr_fb_height()/2, CAMERA_TEST_FINISHED);
	gr_flip();

	ret = ui_draw_handle_softkey(thiz->name);
	thiz->passed = ret;

	return 0;
}

static void camera_test_destroy(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	if (priv->v4l2_fd >= 0) {
		close(priv->v4l2_fd);
	}

	if (priv->fb_fd >= 0) {
		close(priv->fb_fd);
	}

	if (priv->pmem_fd >= 0) {
		close(priv->pmem_fd);
	}

	SAFE_FREE(priv->tmpbuf);
	SAFE_FREE(thiz);
}

TestCase* camera_test_case_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		char buf[8];
		DECLES_PRIV(priv, thiz);

		thiz->run = camera_test_run;
		thiz->destroy = camera_test_destroy;

		thiz->name = CAMERA_TEST_CASE;
		thiz->passed = passed;

		memset(buf, 0, sizeof(buf));
		property_get(HWROTATION_PROP, buf, "0");

		if (!strncmp(buf, "180", 3)) {
			priv->reverse_screen = 1;
		} else {
			priv->reverse_screen = 0;
		}

		priv->fb_fd = -1;
		priv->v4l2_fd = -1;
		priv->pmem_fd = -1;
		priv->camera_preview_run = 0;
		priv->only_back_camera = 0;
		priv->page_offset_align = 0;
		priv->tmpbuf = NULL;
		priv->pmem_base = NULL;
		priv->fb_bits = NULL;
	}

	return thiz;
}
