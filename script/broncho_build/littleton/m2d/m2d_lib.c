/*
 * INTEL CONFIDENTIAL
 * Copyright 2005-2006 Intel Corporation All Rights Reserved.
 * 
 * The source code contained or described herein and all documents
 * related to the source code (Material) are owned by Intel Corporation
 * or its suppliers or licensors.  Title to the Material remains with
 * Intel Corporation or its suppliers and licensors. The Material contains
 * trade secrets and proprietary and confidential information of Intel
 * or its suppliers and licensors. The Material is protected by worldwide
 * copyright and trade secret laws and treaty provisions. No part of the
 * Material may be used, copied, reproduced, modified, published, uploaded,
 * posted, transmitted, distributed, or disclosed in any way without Intel's
 * prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or
 * delivery of the Materials, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights
 * must be express and approved by Intel in writing.

 *(C) Copyright 2006 Marvell International Ltd.  
 * All Rights Reserved 
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/uio.h>
#include <linux/types.h>
#include <errno.h>

#include <linux/fb.h>
#include <string.h>
#include "m2d_drv.h"
#include "m2d_lib.h"

/*
#define CONFIG_IPM_M2D_WORKAROUND
*/

#ifdef  CONFIG_IPM_M2D_WORKAROUND
#include "ipm-lib.h"
#endif

/*
#define CONFIG_GCU_LINE_WORKAROUND
*/

#ifdef __GNUC__
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#else
#define likely(x)	x
#define unlikely(x)	x
#endif

#define M2D_DEVICE_FILE	"/dev/m2d"

#define CONFIG_CHECK_BOUNDARY

/* #define DEBUG */

#ifdef  DEBUG
  #define pr_debug(fmt, arg...) printf(fmt, ##arg)
#else
  #define pr_debug(fmt, arg...) do { } while(0)
#endif

typedef enum {
	GC_BUFFI_ADDR_SRC0      = 0x0,
	GC_BUFFI_ADDR_SRC1      = 0x1,
	GC_BUFFI_ADDR_DST0      = 0x8,
	GC_BUFFI_ADDR_DST1      = 0x9,
	GC_BUFFI_ADDR_DST2      = 0xa,
} GCU_BUFFI_ADDR;

struct m2d_buff {
	uint32_t	bus_addr; /* bus address */
	uint32_t	format;   /* pixel format */
	uint32_t	stride;   /* size of a scan line */
	uint32_t	step;     /* size of a pixel */	

	void *		addr;     /* address application can access */
	int		width;    /* width & height of the rectangle */
	int		height;   /* this buffer represent */
	size_t		size;	  /* size of the buffer */
	
	struct m2d_buff *parent;  /* != NULL if this is a sub buffer */
};

void * m2d_buff_addr(struct m2d_buff *buf)
{
	return (void *)(buf->addr);
}

size_t m2d_buff_size(struct m2d_buff *buf)
{
	return (size_t)(buf->size);
}

#define CMDBUF_SAFE_ROOM	0x40
#define CMDBUF_SIZE		(4096)

struct m2d_cmdbuf {
	unsigned long	cb_len;
	uint32_t *	cb_ptr;
	unsigned char	cb_data[CMDBUF_SIZE];
};

#define SCRATCH_BUFF_SIZE	4096

struct m2d_context {
	int		m2d_fd;

	GCU_PIXEL_FORMAT fill_color_format;
	GCU_COLOR_VALUE  fill_color_value;

	GCU_PIXEL_FORMAT line_color_format;
	GCU_COLOR_VALUE  line_color_value;
	int		line_width;

	GCU_PIXEL_FORMAT aaline_color_format;
	GCU_COLOR_VALUE  aaline_color_value;
	int		aaline_width;
	unsigned char	*aaline_mask_addr;
	unsigned long	aaline_mask_bus_addr;

	GCU_BLEND_OP	blend_op;
	GCU_BIAS_OP	bias_op;
	int		ckblt_invert;

#ifdef CONFIG_GCU_LINE_WORKAROUND
	GCU_PIXEL_FORMAT scratch_reg_format[8];
	GCU_COLOR_VALUE  scratch_reg_color[8];
#endif

	struct m2d_buff *scratch_buff;

	struct m2d_buff *srcbuf0;
	struct m2d_buff *srcbuf1;
	struct m2d_buff	*dstbuf;

	struct m2d_cmdbuf	cmdbuf;
};

#ifdef  CONFIG_IPM_M2D_WORKAROUND
#define SYSOP_BUF	10
static int	m2d_count = 0;	/* It's used to calculate how many times 2d library is opened. */
static AppID	m2d_id;		/* It's used to save the id number. */
#endif

/****************************************************************************
 * Private Function Prototypes
 ***************************************************************************/
static inline int _m2d_load_buffi(
		struct m2d_context *ctx,
		struct m2d_buff *buffi,
		GCU_BUFFI_ADDR which_buff);

static int _m2d_submit(struct m2d_context *ctx, int submit_mode);

/****************************************************************************
 * User Space Context Management
 ***************************************************************************/

struct m2d_context *m2d_create_context(void)
{
	int m2d_fd;
	struct m2d_context *ctx;
	struct m2d_buff *sbuf;
	static uint8_t default_aaline_mask[8] =
		{0x33, 0x66, 0x99, 0xCC, 0xDD, 0xAA, 0x77, 0x44};

	/* Allocate memory for the context structure */
	if ((ctx = malloc(sizeof(struct m2d_context))) == NULL) {
		pr_debug("failed to allocate memory for context\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(*ctx));

	/* Open the device and acquire a new context */
	if ((ctx->m2d_fd = open(M2D_DEVICE_FILE, O_RDWR)) < 0) {
		pr_debug("failed to open 2D device file: %s\n",
				M2D_DEVICE_FILE);
		goto fail_free;
	}

#ifdef  CONFIG_IPM_M2D_WORKAROUND
	if (++m2d_count == 1) {
		int sys_fd = 0, ret = 0, count = 0;;
		char buf[SYSOP_BUF];
		m2d_id = IPMApplicationRegister(IPMAPPTYPE_VIDEO);
		do {
			count++;
			sleep(1);
			if (sys_fd == 0)
				sys_fd = open("/sys/devices/system/cpu/cpu0/op", O_RDONLY);
			if (sys_fd < 0) {
				/* can't get system info */
				break;
			}
			else {
				memset(buf, 0, SYSOP_BUF);
				ret = read(sys_fd, buf, SYSOP_BUF);
				if (ret < 0) {
					/* can't get system info, read again */
					if (count > 10)
						/* wait too long, don't wait again. */
						break;
					else
						continue;
				}
				/* skip 208MHz */
				if (!strncmp(buf, "op:2", 4))
					continue;
				else {
					/* valid op */
					break;
				}
			} 
		} while (1);
	}
#endif

	/* Initialize the instruction buffer */
	ctx->cmdbuf.cb_len = 0;
	ctx->cmdbuf.cb_ptr = (uint32_t *)(ctx->cmdbuf.cb_data);
	memset(ctx->cmdbuf.cb_data, 0, CMDBUF_SIZE);

	/* Allocate a scratch buffer for internal use */
	sbuf = m2d_alloc_buffer(
			ctx, SCRATCH_BUFF_SIZE,
			1, GCU_PXLFMT_INDEXED_8);
	if (sbuf == NULL) {
		pr_debug("failed to allocate scratch buffer\n");
		goto fail_close;
	}

	ctx->scratch_buff     = sbuf;
	ctx->aaline_mask_addr = sbuf->addr;
	ctx->aaline_mask_bus_addr = sbuf->bus_addr;

	/* Initialize the default settings */
	m2d_set_fill_color(ctx, GCU_PXLFMT_RGB565, 0LL);
	m2d_set_line_color(ctx, GCU_PXLFMT_RGB565, 0LL);
	m2d_set_aaline_color(ctx, GCU_PXLFMT_RGB565, 0LL);

	m2d_set_line_width(ctx, 1);
	m2d_set_aaline_width(ctx, 1);
	m2d_set_aaline_mask (ctx, default_aaline_mask);

	m2d_set_blend_op(ctx, GCU_BLEND_OP_ADD);
	m2d_set_bias_op (ctx, GCU_BIAS_OP_ADD);
	m2d_set_ckblt_invert(ctx, 0);

	ctx->srcbuf0 = NULL;
	ctx->srcbuf1 = NULL;
	ctx->dstbuf  = NULL;

	return ctx;

fail_close:
#ifdef  CONFIG_IPM_M2D_WORKAROUND
	if (--m2d_count == 0)
		IPMApplicationExit(m2d_id);
#endif
	close(m2d_fd);

fail_free:
	free(ctx);
	return NULL;
}

void m2d_free_context(struct m2d_context *ctx)
{
	if (ctx == NULL) return;

#ifdef  CONFIG_IPM_M2D_WORKAROUND
	if (--m2d_count == 0)
		IPMApplicationExit(m2d_id);
#endif
	close(ctx->m2d_fd);
	m2d_free_buffer(ctx, ctx->scratch_buff);
	free(ctx);
}

int m2d_set_fill_color(
		struct m2d_context *ctx,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE  c)
{
	if (ctx == NULL) return -1;

	if (unlikely(GCU_PXLFMT_FORMAT(pf) > 0xb))
		return -ERR_IPF;

	ctx->fill_color_format = pf;
	ctx->fill_color_value  = c;

	return 0;
}

int m2d_set_line_color(
		struct m2d_context *ctx,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE  c)
{
	if (ctx == NULL) return -1;

	if (unlikely(GCU_PXLFMT_FORMAT(pf) > 0xb))
		return -ERR_IPF;

	ctx->line_color_format = pf;
	ctx->line_color_value  = c;

	return 0;
}

int m2d_set_aaline_color(
		struct m2d_context *ctx,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE  c)
{
	if (ctx == NULL) return -1;

	if (unlikely(GCU_PXLFMT_FORMAT(pf) > 0xb))
		return -ERR_IPF;

	ctx->aaline_color_format = pf;
	ctx->aaline_color_value  = c;

	return 0;
}

void m2d_set_line_width(struct m2d_context *ctx, int width)
{
	if (ctx) ctx->line_width = width;
}

void m2d_set_aaline_width(struct m2d_context *ctx, int width)
{
	if (ctx) ctx->aaline_width = width;
}

void m2d_set_aaline_mask(struct m2d_context *ctx, uint8_t mask[8])
{
	if (ctx) memcpy(ctx->aaline_mask_addr, mask, 8);
}

void m2d_set_blend_op(struct m2d_context *ctx, GCU_BLEND_OP op)
{
	if (ctx) ctx->blend_op = op;
}

GCU_BLEND_OP m2d_get_blend_op(struct m2d_context *ctx)
{
	return ctx->blend_op;
}

void m2d_set_bias_op(struct m2d_context *ctx, GCU_BIAS_OP op)
{
	if (ctx) ctx->bias_op = op;
}

GCU_BIAS_OP m2d_get_bias_op(struct m2d_context *ctx)
{
	return ctx->bias_op;
}

void m2d_set_ckblt_invert(struct m2d_context *ctx, int invert)
{
	if (ctx) ctx->ckblt_invert = (invert) ? 1 : 0;
}

int m2d_get_ckblt_invert(struct m2d_context *ctx)
{
	return ctx->ckblt_invert;
}

/****************************************************************************
 * Graphics Buffer Management
 ***************************************************************************/

struct m2d_buff *m2d_create_buffer(
		struct m2d_context *ctx,
		int width,
		int height,
		GCU_PIXEL_FORMAT pf,
		void *virt_addr)
{
	int rc = 0;
	struct m2d_buff *buff;
	unsigned long bus_addr = (unsigned long)virt_addr;

	if (ioctl(ctx->m2d_fd, M2DIO_GET_BUS_ADDR, &bus_addr))
	{
		
		return NULL;
	}

	if ((buff = malloc(sizeof(struct m2d_buff))) == NULL)
	{

		return NULL;
	}

	buff->bus_addr	= bus_addr;
	buff->format	= GCU_PXLFMT_FORMAT(pf);
	buff->step	= GCU_PXLFMT_STEP(pf);
	buff->stride	= width * buff->step;

	buff->width	= width;
	buff->height	= height;
	buff->size	= buff->stride * buff->height;
	buff->addr	= virt_addr;
	buff->parent	= NULL;

	return buff;
}

void m2d_destroy_buffer(struct m2d_context *ctx, struct m2d_buff *buff)
{
	if (ctx == NULL || buff == NULL)
		return;

	if (buff == ctx->srcbuf0)
		ctx->srcbuf0 = NULL;

	if (buff == ctx->srcbuf1)
		ctx->srcbuf1 = NULL;

	if (buff == ctx->dstbuf)
		ctx->dstbuf = NULL;

	free(buff);
}

struct m2d_buff *m2d_alloc_buffer(
		struct m2d_context *ctx,
		int width,
		int height,
		GCU_PIXEL_FORMAT pf)
{
	struct m2d_mem_req mem_req;
	struct m2d_buff *buff;

	buff = malloc(sizeof(struct m2d_buff));
	if (buff == NULL)
		return NULL;

	mem_req.req_type = M2D_GRAPHICS_MEM;
	mem_req.req_size = width * height * GCU_PXLFMT_STEP(pf);

	if (ioctl(ctx->m2d_fd, M2DIO_REQUEST_MEM, &mem_req) < 0)
		return NULL;

	buff->bus_addr	= mem_req.phys_addr;
	buff->format	= GCU_PXLFMT_FORMAT(pf);
	buff->step	= GCU_PXLFMT_STEP(pf);
	buff->stride	= width * buff->step;

	buff->width	= width;
	buff->height	= height;
	buff->size	= buff->stride * buff->height;
	buff->addr	= (void *)mem_req.mmap_addr;
	buff->parent	= NULL;

	return buff;
}

/* A sub-buffer represents a sub region of a buffer, it does not
 * have a graphics memory block associated. */
struct m2d_buff* m2d_sub_buffer(struct m2d_buff *buff,
		int x, int y,
		int w, int h)
{
	struct m2d_buff *sbuff;
	unsigned long offset;

	sbuff = malloc(sizeof(struct m2d_buff));
	if (sbuff == NULL)
		return NULL;

	offset = (y * buff->stride) + x * buff->step;
	sbuff->bus_addr	= buff->bus_addr + offset;
	sbuff->format	= buff->format;
	sbuff->step	= buff->step;
	sbuff->stride	= buff->stride;

	sbuff->width	= w;
	sbuff->height	= h;

	sbuff->addr	= buff->addr + offset;
	sbuff->parent	= buff;

	return sbuff;
}

/* FIXME: all sub buffer shall be freed as well
 */
void m2d_free_buffer(struct m2d_context *ctx, struct m2d_buff *buff)
{
	if (buff == ctx->srcbuf0)
		ctx->srcbuf0 = NULL;

	if (buff == ctx->srcbuf1)
		ctx->srcbuf1 = NULL;

	if (buff == ctx->dstbuf)
		ctx->dstbuf = NULL;

	if (buff->parent == NULL)
		ioctl(ctx->m2d_fd, M2DIO_RELEASE_MEM, buff->addr);
	free(buff);
}

struct m2d_buff *m2d_get_primary(void)
{
	static int fbdev = 0;
	static struct m2d_buff primary;
	struct fb_fix_screeninfo fix; 
	struct fb_var_screeninfo var;
	void *addr;

	if (fbdev <= 0) {
		fbdev = open("/dev/fb0", O_RDWR);
		if (fbdev <= 0)
			return NULL;

		ioctl(fbdev, FBIOGET_FSCREENINFO, &fix);
		ioctl(fbdev, FBIOGET_VSCREENINFO, &var);
		primary.bus_addr  = fix.smem_start;
		primary.format    = GCU_PXLFMT_FORMAT(GCU_PXLFMT_RGB565);
		primary.width     = var.xres;
		primary.height    = var.yres;
		primary.step      = var.bits_per_pixel / 8;
		primary.stride    = var.xres * primary.step;

		addr = mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE,
				MAP_SHARED, fbdev, 0);
		if (addr == MAP_FAILED) {
			fbdev = 0;
			return NULL;
		}
		primary.addr   = addr;
		primary.parent = NULL;
	}

	return &primary;
}

int m2d_set_srcbuf0(struct m2d_context *ctx, struct m2d_buff *buffi)
{
	int rc = _m2d_load_buffi(ctx, buffi, GC_BUFFI_ADDR_SRC0);
	if (rc > 0) ctx->srcbuf0 = buffi;
	return rc;
}

int m2d_set_srcbuf1(struct m2d_context *ctx, struct m2d_buff *buffi)
{
	int rc = _m2d_load_buffi(ctx, buffi, GC_BUFFI_ADDR_SRC1);
	if (rc > 0) ctx->srcbuf1 = buffi;
	return rc;
}

int m2d_set_dstbuf0(struct m2d_context *ctx, struct m2d_buff *buffi)
{
	return _m2d_load_buffi(ctx, buffi, GC_BUFFI_ADDR_DST0);
}

int m2d_set_dstbuf1(struct m2d_context *ctx, struct m2d_buff *buffi)
{
	return _m2d_load_buffi(ctx, buffi, GC_BUFFI_ADDR_DST1);
}

int m2d_set_dstbuf2(struct m2d_context *ctx, struct m2d_buff *buffi)
{
	int rc = _m2d_load_buffi(ctx, buffi, GC_BUFFI_ADDR_DST2);
	if (rc > 0) ctx->dstbuf = buffi;
	return rc;
}

/****************************************************************************
 * Instruction Execution Management
 ***************************************************************************/

int _m2d_submit_encoded(struct m2d_context *ctx, void *buf, size_t len)
{
	memcpy(ctx->cmdbuf.cb_ptr, buf, (len << 2));
	ctx->cmdbuf.cb_len += len << 2;
	ctx->cmdbuf.cb_ptr += len;
	return _m2d_submit(ctx, 0);
}

static int _m2d_submit(struct m2d_context *ctx, int submit_mode)
{
	struct m2d_submit_req req;
	unsigned int ret = 0;

	if (ctx->cmdbuf.cb_len) {
		req.mode = submit_mode;
		req.base = ctx->cmdbuf.cb_data;
		req.len	 = ctx->cmdbuf.cb_len;
	
		ctx->cmdbuf.cb_ptr = (uint32_t *)ctx->cmdbuf.cb_data;
		ctx->cmdbuf.cb_len = 0;
repeat:
		if ((ret = ioctl(ctx->m2d_fd, M2DIO_SUBMIT, &req)) == 0)
			return 0;

		if (ret == -EAGAIN)
			goto repeat;
	}

	return ret;
}

int m2d_submit(struct m2d_context *ctx)
{
	return _m2d_submit(ctx, 0);
}

int m2d_submit_nodelay(struct m2d_context *ctx)
{
	return _m2d_submit(ctx, M2D_SUBMIT_MODE_NDELAY);
}

int m2d_sync(struct m2d_context *ctx)
{
	if (ctx->cmdbuf.cb_len)
		return _m2d_submit(ctx, M2D_SUBMIT_MODE_SYNC);
	else
		return ioctl(ctx->m2d_fd, M2DIO_SYNC, 0);
}

/**************************************************************************
 *               APIs for Graphics Controller Instructions
 *************************************************************************/

/**************** Macros for User Instruction Buffer Update **************/
#ifdef  DEBUG
#define DUMP_CMDBUF(start,len)						\
do {									\
	int i; uint32_t *p = (uint32_t *)start;				\
	printf("encoded instruction:\n");				\
	for (i = 0; i < len; i++, p)					\
		printf("%08x\n", *p++);					\
} while (0)
#else
#define DUMP_CMDBUF(p,len)
#endif

#define UPDATE_CMDBUF(context, len)					\
do {									\
	DUMP_CMDBUF((context)->cmdbuf.cb_ptr, (len));			\
	(context)->cmdbuf.cb_ptr += (len);				\
	(context)->cmdbuf.cb_len += ((len) << 2);			\
	if ((context)->cmdbuf.cb_len + CMDBUF_SAFE_ROOM > CMDBUF_SIZE)	\
		_m2d_submit((context), 0);				\
} while (0)

/********************* Macros for boundary checking **********************/

#ifdef  CONFIG_CHECK_BOUNDARY
/* Check if the rectangle start @ (x,y) with (w,h) is within the area
 * of the graphics buffer 'buf'. 'x', 'y', 'w', 'h' are assumed to be
 * positive integer. Width 'w' is right-exclusive and height 'h' is
 * bottom exlusive.
 */
#define CHECK_BOUNDARY(x,y,w,h,buf)					\
do {									\
	if ((buf) == NULL || (x) < 0 || (y) < 0)			\
		return -ERR_OOB;					\
	if (((x) + (w) > (buf)->width) || ((x) >= (buf)->width))	\
		return -ERR_OOB;					\
	if (((y) + (h) > (buf)->height)|| ((y) >= (buf)->height))	\
		return -ERR_OOB;					\
} while(0)

#define CHECK_LINE_BOUNDARY(xa,ya,xb,yb,w,buf)				\
do {									\
	uint32_t delta_x = abs((xb) - (xa));				\
	uint32_t delta_y = abs((yb) - (ya));				\
	if (delta_x || delta_y) {					\
	if (delta_x >= delta_y) {					\
		/* x_dir == 1 */					\
		if ((((ya) - (w)/2) < 0) ||				\
		    (((ya) + (w)/2) >= (buf)->height))			\
			return -ERR_OOB;				\
		if ((((yb) - (w)/2) < 0) ||				\
		    (((yb) + (w)/2) >= (buf)->height))			\
			return -ERR_OOB;				\
	} else {							\
		/* x_dir == 0 */					\
		if ((((xa) - (w)/2) < 0) ||				\
		    (((xa) + (w)/2) >= (buf)->width))			\
			return -ERR_OOB;				\
		if ((((xb) - (w)/2) < 0) ||				\
		    (((xb) + (w)/2) >= (buf)->width))			\
			return -ERR_OOB;				\
	}} else return 0;						\
} while(0)

#else
#define CHECK_BOUNDARY(x,y,w,h,buf)
#define CHECK_LINE_BOUNDARY(xa,ya,xb,yb,w,buf)
#endif

/********* Macros for encoding Control and Memory Instructions ***********/

#define GC_BBST		0x00000000	/* Batch Buffer Start */
#define GC_BBEND	0x01000000	/* Batch Buffer End */
#define GC_BUFFI	0x02000000	/* Buffer Info */
#define GC_LREG		0x04000000	/* Load Register */
#define GC_NOP		0x05000000	/* NOP */
#define GC_DBFLIP	0x06000000	/* Destination Buffer Flip */
#define GC_STREG	0x07000000	/* Store Register */
#define GC_INT		0x08000000	/* Interrupt to Core */
#define GC_WAIT		0x09000000	/* Wait for Event */

#define SETUP_BBST(buf, len, sa)					\
do {									\
	buf[0] = GC_BBST | 0x01; len = 2;				\
	buf[1] = (sa);							\
} while (0)

#define SETUP_BBEND(buf, len)						\
do {									\
	buf[0] = GC_BBEND; len = 1;					\
} while (0)

#define SETUP_BUFFI(buf, len, ba, addr, format, stride, step)		\
do {									\
	buf[0] = GC_BUFFI | (((ba) & 0x1f) << 4) | 0x2; len = 3;	\
	buf[1] = addr;							\
	buf[2] = (((format) & 0xf) << 19) | (((stride) & 0x3fff) << 5)	\
		| ((step) & 0xf);					\
} while (0)

#define SETUP_LREG_IMM(buf, len, regnr, f, c)				\
do {									\
	buf[0] = GC_LREG | (((regnr) & 0x7) << 20) | (((f) & 0xf) << 8);\
	if ((f) < 0xA) {						\
		buf[0] |= 0x11; len = 2;				\
		buf[1] = (uint32_t)(c);					\
	} else if ((f) <= 0xB) {					\
		buf[0] |= 0x12; len = 3;				\
		buf[1] = (uint32_t)((c) >> 32);				\
		buf[2] = (uint32_t)(c);					\
	} else {							\
		len = -ERR_IPF; break;					\
	}								\
} while (0)

#define SETUP_LREG_ADDR(buf, len, regnr, f, addr)			\
do {									\
	buf[0] = GC_LREG | (((regnr) & 0x7) << 20) |			\
		(((f) & 0xf) << 8) | 0x01;				\
	buf[1] = (addr); len = 2;					\
} while (0)

#define SETUP_NOP(buf, len, nop_id)					\
do {									\
	buf[0] = GC_NOP | (((nop_id) & 0xfffff) << 4); len = 1;		\
} while (0)

#define SETUP_DBFLIP(buf, len, mode)					\
do {									\
	buf[0] = GC_DBFLIP | (((mode) & 0x3) << 5); len = 1;		\
} while (0)

#define SETUP_STREG(buf, len, regnr, f, addr)					\
do {									\
	buf[0] = GC_STREG | (((regnr) & 0x7) << 20) |			\
		(((f) & 0xf) << 8) | 0x01;				\
	len = 2;							\
} while (0)

#define SETUP_INT(buf, len, int_id)					\
do {									\
	buf[0] = GC_INT | (((int_id) & 0xfffff) << 4); len = 1;		\
} while (0)

#define SETUP_WAIT(buf, len, event_id)					\
do {									\
	buf[0] = GC_WAIT | (((event_id) & 0x1f) << 4); len = 1;		\
} while (0)

/************ Macros for encoding 2D Graphics Instructions ****************/

#define GC_CFILL_IMM	0x40000010	/* Color Fill (Immediate Color) */
#define GC_CFILL_REG	0x40000000	/* Color Fill (Color in register) */
#define GC_CKBLT	0x41000000	/* Chroma Key BLT */
#define GC_LINE_IMM	0x42000010	/* Line Draw  (Immediate Color) */
#define GC_LINE_REG	0x42000000	/* Line Draw  (Color in register) */
#define GC_AALINE_IMM	0x43000010	/* Anti-alias Line Draw */
#define GC_AALINE_REG	0x43000000	/* Anti-alias Line Draw */
#define GC_STRBLT	0x45000000	/* Stretch BLT */
#define GC_ABLND	0x47000000	/* Alpha Blend BLT */
#define GC_SCALE	0x48000000	/* Scale BLT */
#define GC_BIAS		0x49000000	/* Bias BLT */
#define GC_ROT		0x4a000000	/* Rotate BLT */
#define GC_RASTER	0x4b000000	/* Raster OP BLT */
#define GC_PATT		0x4c000000	/* Pattern Copy BLT */
#define GC_DECBLT	0x4d000000	/* Decimate BLT */

#define SETUP_CFILL_IMM(buf, len, x, y, w, h, pf, c)			\
do {									\
	uint32_t f = GCU_PXLFMT_FORMAT(pf);				\
	if ((f) < 0x0a) {						\
		buf[0] = GC_CFILL_IMM | ((f) << 8) | 0x04; len = 5;	\
		buf[4] = (uint32_t)(c);					\
	} else if ((f) <= 0x0B) {					\
		buf[0] = GC_CFILL_IMM | ((f) << 8) | 0x05; len = 6;	\
		buf[4] = (uint32_t)(c >> 32);				\
		buf[5] = (uint32_t)(c);					\
	} else {							\
		len = -ERR_IPF; break;					\
	}								\
	buf[1] = (x); buf[2] = (y);					\
	buf[3] = ((h) << 16) | ((w) & 0xffff);				\
} while (0)

#define SETUP_CFILL_REG(buf, len, x, y, w, h, regnr)			\
do {									\
	buf[0] = GC_CFILL_REG | (((regnr) & 7) << 20) | 0x03; len = 4;	\
	buf[1] = (x); buf[2] = (y);					\
	buf[3] = ((h) << 16) | ((w) & 0xffff);				\
	len = 4;							\
} while (0)

#define SETUP_LINE_CORE(buf, x0, y0, x1, y1, w)				\
do {									\
	uint32_t delta_x = abs((x1) - (x0));				\
	uint32_t delta_y = abs((y1) - (y0));				\
	buf[1] = x0; buf[2] = y0;					\
	buf[3] = 0;  buf[4] = ((w) & 0xff) << 16;			\
	if (((x1) - (x0)) >= 0) buf[3] |= 0x80000000;			\
	if (((y0) - (y1)) >= 0) buf[3] |= 0x00008000;			\
	if (delta_x > delta_y) {					\
		buf[3] |= (0x40000000);					\
		buf[3] |= ((delta_y << 14) / delta_x) & 0x3fff;		\
		buf[4] |= (delta_x + 1) & 0x7ff;			\
	} else if (delta_x == delta_y) {				\
		buf[3] |= (0x40004000);					\
		buf[4] |= (delta_x + 1) & 0x7ff;			\
	} else {							\
		buf[3] |= (0x00004000);					\
		buf[3] |= (((delta_x << 14) / delta_y) & 0x3fff) << 16; \
		buf[4] |= (delta_y + 1) & 0x7ff;			\
	}								\
} while (0)

#define SETUP_LINE_IMM(buf, len, x0, y0, x1, y1, pf, c, w)		\
do {									\
	uint32_t f = GCU_PXLFMT_FORMAT(pf);				\
	if ((f) < 0xA) {						\
		buf[0] = GC_LINE_IMM | ((f) << 8) | 0x05; len = 6;	\
		buf[5] = (uint32_t)(c);					\
	} else if ((f) <= 0xB) {					\
		buf[0] = GC_LINE_IMM | ((f) << 8) | 0x06; len = 7;	\
		buf[5] = (uint32_t)((c) >> 32);				\
		buf[6] = (uint32_t)(c);					\
	} else {							\
		len = -ERR_IPF; break;					\
	}								\
	SETUP_LINE_CORE(buf, x0, y0, x1, y1, w);			\
} while (0)

#define SETUP_LINE_REG(buf, len, x0, y0, x1, y1, regnr, w)		\
do {									\
	buf[0] = GC_LINE_REG | (((regnr) & 7) << 20) | 0x04; len = 5;	\
	SETUP_LINE_CORE(buf, x0, y0, x1, y1, w);			\
} while (0)

#define SETUP_AALINE_IMM(buf, len, x0, y0, x1, y1, pf, c, w, ma)	\
do {									\
	uint32_t f = GCU_PXLFMT_FORMAT(pf);				\
	if ((f) < 0xA) {						\
		buf[0] = GC_AALINE_IMM | ((f) << 8) | 0x6; len = 7;	\
		buf[6] = (uint32_t)(c);					\
	} else if ((f) <= 0xB) {					\
		buf[0] = GC_AALINE_IMM | ((f) << 8) | 0x7; len = 8;	\
		buf[6] = (uint32_t)((c) >> 32);				\
		buf[7] = (uint32_t)(c);					\
	} else {							\
		len = -ERR_IPF; break;					\
	}								\
	buf[5] = (ma);							\
	SETUP_LINE_CORE(buf, x0, y0, x1, y1, w);			\
} while (0)

#define SETUP_AALINE_REG(buf, len, x0, y0, x1, y1, regnr, w, ma)	\
do {									\
	buf[0] = GC_AALINE_REG | (((regnr) & 7) << 20) | 0x5; len = 6;	\
	buf[5] = (ma);							\
	SETUP_LINE_CORE(buf, x0, y0, x1, y1, w);			\
} while (0)

#define SETUP_SCALE(buf, len, imm, dx, dy, sx, sy, w, h, scf)		\
do {									\
	if (imm) {							\
		buf[0] = GC_SCALE | 0x16; len = 7;			\
		buf[6] = (scf) & 0xffff;				\
	} else {							\
		buf[0] = GC_SCALE | 0x05; len = 6;			\
	}								\
	buf[1] = dx; buf[2] = dy;					\
	buf[3] = sx; buf[4] = sy;					\
	buf[5] = ((h) << 16) | ((w) & 0xffff);				\
} while (0)

#define SETUP_BIAS(buf, len, imm, op, dx, dy, sx, sy, w, h, bias)	\
do {									\
	if (imm) {							\
		buf[0] = GC_BIAS | (((op) & 1) << 5) | 0x16; len = 7;	\
		buf[6] = (bias) & 0xffff;				\
	} else {							\
		buf[0] = GC_BIAS | (((op) & 1) << 5) | 0x05; len = 6;	\
	}								\
	buf[1] = dx; buf[2] = dy;					\
	buf[3] = sx; buf[4] = sy;					\
	buf[5] = ((h) << 16) | ((w) & 0xffff);				\
} while (0)

#define SETUP_ABLND(buf, len, imm, s, op, dx, dy, x0, y0, x1, y1, w, h, a0, a1) \
do {									\
	buf[0] = GC_ABLND | (((op) & 3) << 5) | (((s) & 1) << 8);	\
	if (imm) {							\
		buf[0] |= 0x08 | ((imm) << 4); len = 9; buf[8] = 0;	\
		if ((imm) & 0x1) { buf[8] |= (a0) << 16; }		\
		if ((imm) & 0x2) { buf[8] |= (a1) & 0xffff; }		\
	} else {							\
		buf[0] |= 0x07; len = 8;				\
	}								\
	buf[1] = dx; buf[2] = dy;					\
	buf[3] = x0; buf[4] = y0;					\
	buf[5] = x1; buf[6] = y1;					\
	buf[7] = ((h) << 16) | ((w) & 0xffff);				\
} while (0)

#define SETUP_STRBLT(buf, len, dx, dy, sx, sy, sw, sh, dw, dh)		\
do {									\
	uint32_t x_str = (((sw) - 1) << 16) / ((dw) - 1);		\
	uint32_t y_str = (((sh) - 1) << 16) / ((dh) - 1);		\
	buf[0] = GC_STRBLT | 0x8; len = 9;				\
	buf[1] = dx; buf[2] = dy;					\
	buf[3] = sx; buf[4] = sy;					\
	buf[5] = (sh) << 16 | (sw) & 0x1ff;				\
	buf[6] = (dh) << 16 | (dw) & 0xffff;				\
	buf[7] = (x_str & 0x03ff0000) | ((x_str >> 6) & 0x3ff);		\
	buf[8] = (y_str & 0x03ff0000) | ((y_str >> 6) & 0x3ff);		\
} while (0)

#define SETUP_DECBLT(buf, len, dx, dy, sx, sy, sw, sh, dw, dh)		\
do {									\
	uint32_t x_dec = (((sw) - 1) << 16) / ((dw) - 1);		\
	uint32_t y_dec = (((sh) - 1) << 16) / ((dh) - 1);		\
	buf[0] = GC_DECBLT | 0x8; len = 9;				\
	buf[1] = dx; buf[2] = dy;					\
	buf[3] = sx; buf[4] = sy;					\
	buf[5] = (sh) << 16 | (sw) & 0x1ff;				\
	buf[6] = (dh) << 16 | (dw) & 0xffff;				\
	buf[7] = (x_dec & 0x03ff0000) | ((x_dec >> 6) & 0x3ff);		\
	buf[8] = (y_dec & 0x03ff0000) | ((y_dec >> 6) & 0x3ff);		\
} while (0)

#define SETUP_CKBLT(buf, len, inv, dx, dy, sx, sy, w, h, t, k)		\
do {									\
	buf[0] = GC_CKBLT | ((inv) << 4) | 0x06; len = 7;		\
	buf[1] = dx; buf[2] = dy;					\
	buf[3] = sx; buf[4] = sy;					\
	buf[5] = ((h) << 16) | ((w) & 0xffff);				\
	buf[6] = (((k) & 0x3) << 17) | (t & 0x1ffff);			\
} while (0)

#define SETUP_RASTER(buf, len, rop, dx, dy, x0, y0, x1, y1, w, h)	\
do {									\
	buf[0] = GC_RASTER | ((rop) << 16) | 0x07; len = 8;		\
	buf[1] = dx; buf[2] = dy;					\
	buf[3] = x0; buf[4] = y0;					\
	buf[5] = x1; buf[6] = y1;					\
	buf[7] = ((h) << 16) | ((w) & 0xffff);				\
} while (0)

#define SETUP_PATT(buf, len, opt, dx, dy, sx, sy, sw, sh, dw, dh)	\
do {									\
	buf[0] = GC_PATT | ((opt) << 4) | 0x06; len = 7;		\
	buf[1] = dx; buf[2] = dy;					\
	buf[3] = sx; buf[4] = sy;					\
	buf[5] = ((sh) << 16) | ((sw) & 0xffff);			\
	buf[6] = ((dh) << 16) | ((dw) & 0xffff);			\
} while (0)

#define SETUP_ROTATE(buf, len, rot, dx, dy, sx, sy, w, h)		\
do {									\
	buf[0] = GC_ROT | (((rot) & 0x3) << 4) | 0x5; len = 6;		\
	buf[1] = dx; buf[2] = dy;					\
	buf[3] = sx; buf[4] = sy;					\
	buf[5] = ((h) << 16) | ((w) & 0xffff);				\
} while (0)
static inline int _m2d_load_buffi(
		struct m2d_context *ctx,
		struct m2d_buff *buffi,
		GCU_BUFFI_ADDR which_buff)
{
	int len = 0;

	if (ctx == NULL || buffi == NULL)
		return -1;

	SETUP_BUFFI(ctx->cmdbuf.cb_ptr, len, which_buff,
			buffi->bus_addr,
			buffi->format,
			buffi->stride,
			buffi->step);
	UPDATE_CMDBUF(ctx, len);
	//m2d_sync(ctx);
	return len;
}

int m2d_load_reg(
		struct m2d_context *ctx,
		int regnr,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE color)
{
	int len = 0;

	if (ctx == NULL) return -1;
	if (regnr < 0 || regnr > 7)
		return -ERR_REG;

#ifdef CONFIG_GCU_LINE_WORKAROUND
	ctx->scratch_reg_format[regnr] = pf;
	ctx->scratch_reg_color[regnr]  = color;
#endif

	SETUP_LREG_IMM(ctx->cmdbuf.cb_ptr, len, regnr,
			GCU_PXLFMT_FORMAT(pf), color);
	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_nop(struct m2d_context *ctx, int nop_id)
{
	int len = 0;
	if (ctx == NULL) return -1;

	SETUP_NOP(ctx->cmdbuf.cb_ptr, len, nop_id);
	UPDATE_CMDBUF(ctx, len);
	return len;
}

/***************************** Color Fill *********************************/

int m2d_color_fill(
		struct m2d_context *ctx,
		struct m2d_op_region *opr)
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY( opr->dx0, opr->dy0,
			opr->width, opr->height,
			ctx->dstbuf);

	SETUP_CFILL_IMM(ctx->cmdbuf.cb_ptr, len,
			opr->dx0, opr->dy0,
			opr->width, opr->height,
			ctx->fill_color_format,
			ctx->fill_color_value);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_color_fill_imm(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE  color_or_regnr)
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY( opr->dx0, opr->dy0,
			opr->width, opr->height,
			ctx->dstbuf);
	
	if (pf == GCU_PXLFMT_REGISTER)
		SETUP_CFILL_REG(ctx->cmdbuf.cb_ptr, len,
				opr->dx0, opr->dy0,
				opr->width, opr->height,
				(int)color_or_regnr);
	else
		SETUP_CFILL_IMM(ctx->cmdbuf.cb_ptr, len,
				opr->dx0, opr->dy0,
				opr->width, opr->height,
				pf, color_or_regnr);

	UPDATE_CMDBUF(ctx, len);

	return len;
}

#ifdef CONFIG_GCU_LINE_WORKAROUND

static inline void *generate_line_pattern(
		struct m2d_context *ctx,
		int *pattern_len,
		int  width,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE  color_or_regnr,
		uint8_t mask[8])
{
	GCU_PIXEL_FORMAT format;
	GCU_COLOR_VALUE  color;
	uint8_t r, g, b;
	unsigned char *pattern_buf;
	uint16_t *p;
	int i;

	if (pf == GCU_PXLFMT_REGISTER) {
		int regnr = color_or_regnr % 8;
		format = ctx->scratch_reg_format[regnr];
		color  = ctx->scratch_reg_color[regnr];
	} else {
		format = pf;
		color  = color_or_regnr;
	}

	/* destination buffer in RGB565 support only */
	if (ctx->dstbuf->format != GCU_PXLFMT_FORMAT(GCU_PXLFMT_RGB565)) {
		printf("dstbuf other than RGB565 not supported\n");
		return NULL;
	}

	if (mask == NULL) {
		*pattern_len = width << 1;
		 pattern_buf = malloc(width << 1);
	} else {
		*pattern_len = (width + 8) << 1;
		 pattern_buf = malloc((width + 8) << 1);
	}
	
	p = (uint16_t *)pattern_buf;

	switch (format) {
	case GCU_PXLFMT_RGB555:
	{
		r = (uint8_t)(((uint16_t)(color) & 0x7c00) >> 7);
		g = (uint8_t)(((uint16_t)(color) & 0x03e0) >> 2);
		b = (uint8_t)(((uint16_t)(color) & 0x001f) << 3);

		break;
	}

	case GCU_PXLFMT_RGB565:
	{
		r = (uint8_t)(((uint16_t)(color) & 0xf800) >> 8);
		g = (uint8_t)(((uint16_t)(color) & 0x07e0) >> 3);
		b = (uint8_t)(((uint16_t)(color) & 0x001f) << 3);

		break;
	}

	case GCU_PXLFMT_RGBA8888:
	{
		r = (uint8_t)(((uint32_t)(color) & 0x00ff0000) >> 16);
		g = (uint8_t)(((uint32_t)(color) & 0x0000ff00) >> 8);
		b = (uint8_t)(((uint32_t)(color) & 0x000000ff));

		break;
	}

	default:
		printf("%s: pf = %x not supported in LINE\n", __FUNCTION__, format);
		free(pattern_buf);
		return NULL;
	}

	if (mask) {
		for (i = 0; i < 4; i++)
		*p++ =  ((((uint16_t)r * mask[i] / 0xFF) & 0xf8) << 8) |
			((((uint16_t)g * mask[i] / 0xFF) & 0xfc) << 3) |
			((((uint16_t)b * mask[i] / 0xFF) & 0xf8) >> 3);
	}

	for (i = 0; i < width; i++)
		*p++ =  (((uint16_t)r & 0xf8) << 8) |
			(((uint16_t)g & 0xfc) << 3)  |
			(((uint16_t)b & 0xf8) >> 3);

	if (mask) {
		for (i = 4; i < 8; i++)
		*p++ =  ((((uint16_t)r * mask[i] / 0xFF) & 0xf8) << 8) |
			((((uint16_t)g * mask[i] / 0xFF) & 0xfc) << 3) |
			((((uint16_t)b * mask[i] / 0xFF) & 0xf8) >> 3);
	}

	return pattern_buf;
}

static inline int fill_pattern_h(
		struct m2d_buff *buf,
		unsigned long  off,
		unsigned char *pattern,
		int pattern_len)
{
	unsigned long addr = (unsigned long)(buf->addr) + off;
	memcpy((void *)addr, pattern, pattern_len);
	return 0;
}

static inline int fill_pattern_v(
		struct m2d_buff *buf,
		unsigned long  off,
		unsigned char *pattern,
		int pattern_len)
{
	unsigned long addr = (unsigned long)(buf->addr) + off;

	if (buf->step == 2) {
		uint16_t *src = (uint16_t *)pattern;
		do {
			*(uint16_t *)(addr) = *src;
			addr += buf->stride;
			src++;
			pattern_len -= 2;
		} while(pattern_len > 0);

		return 0;
	}

	if (buf->step == 1) {
		uint8_t *src = (uint8_t *)pattern;
		do {
			*(uint8_t *)(addr) = *src;
			addr += buf->stride;
			src++;
			pattern_len -= 1;
		} while(pattern_len > 0);

		return 0;
	}

	if (buf->step == 4) {
		uint32_t *src = (uint32_t *)pattern;
		do {
			*(uint32_t *)(addr) = *src;
			addr += buf->stride;
			src++;
			pattern_len -= 4;
		} while(pattern_len > 0);

		return 0;
	}

	return -ERR_IPF;
}

static inline void m2d_draw_line_core_x(
		struct m2d_buff *buf,
		int x0, int y0,
		int delta_x,
		int delta_y,
		int step_x,
		int step_y,
		unsigned char *pattern,
		int pattern_len)
{
	unsigned long off = y0 * buf->stride + x0 * buf->step;
	int i, frac;

	delta_x <<= 1;
	delta_y <<= 1;

	frac = delta_y - (delta_x >> 1);

	fill_pattern_v(buf, off, pattern, pattern_len);
	for (i = 0; i < (delta_x >> 1); i++) {
		if (frac >= 0) {
			/* y0 += step_y */
			frac -= delta_x;
			off  += step_y * buf->stride;
		}
		/* x0 += step_x; */
		frac += delta_y;
		off  += step_x * buf->step;
		fill_pattern_v(buf, off, pattern, pattern_len);
	}
}

static inline void m2d_draw_line_core_y(
		struct m2d_buff *buf,
		int x0, int y0,
		int delta_x,
		int delta_y,
		int step_x,
		int step_y,
		unsigned char *pattern,
		int pattern_len)
{
	unsigned long off = y0 * buf->stride + x0 * buf->step;
	int i, frac;

	delta_x <<= 1;
	delta_y <<= 1;

	frac = delta_x - (delta_y >> 1);

	fill_pattern_h(buf, off, pattern, pattern_len);
	for (i = 0; i < (delta_y >> 1); i++) {
		if (frac >= 0) {
			/* x0 += step_x */
			frac -= delta_y;
			off  += step_x * buf->step;
		}
		/* y0 += step_y; */
		frac += delta_x;
		off  += step_y * buf->stride;
		fill_pattern_h(buf, off, pattern, pattern_len);
	}
}

int m2d_draw_line_soft(
		struct m2d_context *context,
		int x0, int y0,
		int x1, int y1,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE  color_or_reg,
		int width,
		uint8_t mask[8])
{
	int delta_x = x1 - x0;
	int delta_y = y1 - y0;
	int step_x, step_y;
	int pattern_len;
	unsigned char *pattern_buf;
	struct m2d_buff *buf = context->dstbuf;

	if (delta_y < 0) {
		delta_y = - delta_y;
		step_y  = - 1;
	} else
		step_y  = + 1;

	if (delta_x < 0) {
		delta_x = - delta_x;
		step_x  = - 1;
	} else
		step_x  = + 1;

	if (delta_x == 0 && delta_y == 0)
		return 0;

	pattern_buf = generate_line_pattern(context, &pattern_len,
						width, pf, color_or_reg, mask);
	if (pattern_buf == NULL)
		return -ERR_IPF;

#if 0
	{
		uint16_t *p = pattern_buf;
		int i;

		printf("[");
		for (i = 0; i < pattern_len >> 1; i++)
			printf(" %4x", *p++);
		printf("]\n");
	}
#endif
	/* we have to be sure that all the GCU instructions are exhausted */
	m2d_sync(context);

	if (delta_x >= delta_y) {
		int y_adj = y0 - ((mask)? ((width + 8)/2) : (width/2));
		m2d_draw_line_core_x( buf,
			x0, y_adj,
			delta_x,
			delta_y,
			step_x,
			step_y,
			pattern_buf,
			pattern_len);
	} else {
		int x_adj = x0 - ((mask)? ((width + 8)/2) : (width/2));
		m2d_draw_line_core_y( buf,
			x_adj, y0,
			delta_x,
			delta_y,
			step_x,
			step_y,
			pattern_buf,
			pattern_len);
	}

	free(pattern_buf);

	return 6;
}
#endif

int m2d_draw_line(
		struct m2d_context *ctx,
		int x0, int y0,
		int x1, int y1)
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY(x0, y0, 0, 0, ctx->dstbuf);
	CHECK_BOUNDARY(x1, y1, 0, 0, ctx->dstbuf);
	CHECK_LINE_BOUNDARY(x0, y0, x1, y1, ctx->line_width, ctx->dstbuf);

#ifndef CONFIG_GCU_LINE_WORKAROUND
	SETUP_LINE_IMM(ctx->cmdbuf.cb_ptr, len,
			x0, y0, x1, y1,
			ctx->line_color_format,
			ctx->line_color_value,
			ctx->line_width);

	UPDATE_CMDBUF(ctx, len);
#else
	len = m2d_draw_line_soft(ctx, x0, y0, x1, y1,
			ctx->line_color_format,
			ctx->line_color_value,
			ctx->line_width,
			NULL);
#endif
	return len;
}

int m2d_draw_line_imm(
		struct m2d_context *ctx,
		int x0, int y0,
		int x1, int y1,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE color_or_regnr,
		int width)
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY(x0, y0, 0, 0, ctx->dstbuf);
	CHECK_BOUNDARY(x1, y1, 0, 0, ctx->dstbuf);
	CHECK_LINE_BOUNDARY(x0, y0, x1, y1, width, ctx->dstbuf);

#ifndef CONFIG_GCU_LINE_WORKAROUND
	if (pf == GCU_PXLFMT_REGISTER)
		SETUP_LINE_REG(ctx->cmdbuf.cb_ptr, len,
				x0, y0, x1, y1,
				(int)color_or_regnr, width);
	else
		SETUP_LINE_IMM(ctx->cmdbuf.cb_ptr, len,
				x0, y0, x1, y1,
				pf, color_or_regnr, width);
	UPDATE_CMDBUF(ctx, len);
#else
	len = m2d_draw_line_soft(ctx, x0, y0, x1, y1,
			pf, color_or_regnr, width, NULL);
#endif
	return len;
}

int m2d_draw_aaline(
		struct m2d_context *ctx,
		int x0, int y0,
		int x1, int y1)
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY(x0, y0, 0, 0, ctx->dstbuf);
	CHECK_BOUNDARY(x1, y1, 0, 0, ctx->dstbuf);
	CHECK_LINE_BOUNDARY(x0, y0, x1, y1,
			ctx->aaline_width + 8,
			ctx->dstbuf);

#ifndef CONFIG_GCU_LINE_WORKAROUND
	SETUP_AALINE_IMM(ctx->cmdbuf.cb_ptr, len,
			x0, y0, x1, y1,
			ctx->aaline_color_format,
			ctx->aaline_color_value,
			ctx->aaline_width,
			ctx->aaline_mask_bus_addr);

	UPDATE_CMDBUF(ctx, len);
#else
	len = m2d_draw_line_soft(ctx, x0, y0, x1, y1,
			ctx->aaline_color_format,
			ctx->aaline_color_value,
			ctx->aaline_width,
			ctx->aaline_mask_addr);
#endif
	return len;
}

int m2d_draw_aaline_imm(
		struct m2d_context *ctx,
		int x0, int y0,
		int x1, int y1,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE color_or_regnr,
		int width,
		uint8_t mask_array[8])
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY(x0, y0, 0, 0, ctx->dstbuf);
	CHECK_BOUNDARY(x1, y1, 0, 0, ctx->dstbuf);
	CHECK_LINE_BOUNDARY(x0, y0, x1, y1,
			width + 8,
			ctx->dstbuf);

#ifndef CONFIG_GCU_LINE_WORKAROUND
	m2d_set_aaline_mask(ctx, mask_array);

	if (pf == GCU_PXLFMT_REGISTER)
		SETUP_AALINE_REG(ctx->cmdbuf.cb_ptr, len,
				x0, y0, x1, y1,
				(unsigned int)color_or_regnr,
				width,
				ctx->aaline_mask_bus_addr);
	else
		SETUP_AALINE_IMM(ctx->cmdbuf.cb_ptr, len,
				x0, y0, x1, y1,
				pf, color_or_regnr,
				width, ctx->aaline_mask_bus_addr);

	UPDATE_CMDBUF(ctx, len);
#else
	len = m2d_draw_line_soft(ctx, x0, y0, x1, y1,
			pf,
			color_or_regnr,
			width,
			mask_array);
#endif
	return len;
}

int m2d_stretch_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		int dw,  int dh)
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY( opr->s0x0, opr->s0y0,
			opr->width, opr->height,
			ctx->srcbuf0);
	CHECK_BOUNDARY( opr->dx0, opr->dy0,
			dw, dh,
			ctx->dstbuf);

	SETUP_STRBLT( ctx->cmdbuf.cb_ptr, len,
			opr->dx0,  opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->width,opr->height,
			dw, dh);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_decimate_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		int dw, int dh)
{

	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY( opr->s0x0, opr->s0y0,
			opr->width, opr->height,
			ctx->srcbuf0);
	CHECK_BOUNDARY( opr->dx0, opr->dy0,
			dw, dh,
			ctx->dstbuf);

	SETUP_DECBLT( ctx->cmdbuf.cb_ptr, len,
			opr->dx0,  opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->width,opr->height,
			dw, dh);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_scale_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		unsigned int scale_factor)
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY( opr->s0x0, opr->s0y0,
			opr->width,opr->height,
			ctx->srcbuf0);
	CHECK_BOUNDARY( opr->dx0, opr->dy0,
			opr->width, opr->height,
			ctx->dstbuf);

	SETUP_SCALE(ctx->cmdbuf.cb_ptr, len,
			((scale_factor == USE_SRC_ALPHA) ? 0 : 1),
			opr->dx0,  opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->width,opr->height,
			scale_factor);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_chroma_key_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		GCU_CKBLT_KEY key,
		unsigned int threshold)
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY(	opr->s0x0,  opr->s0y0,
			opr->width, opr->height,
			ctx->srcbuf0);
	CHECK_BOUNDARY(	opr->dx0,   opr->dy0,
			opr->width, opr->height,
			ctx->dstbuf);

	SETUP_CKBLT(	ctx->cmdbuf.cb_ptr, len, ctx->ckblt_invert,
			opr->dx0, opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->width, opr->height,
			threshold, key);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_alpha_blend(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		unsigned int alpha0,
		unsigned int alpha1)
{
	int len = 0, imm = 3;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY( opr->s0x0,  opr->s0y0,
			opr->width, opr->height,
			ctx->srcbuf0);
	CHECK_BOUNDARY( opr->s1x0,  opr->s1y0,
			opr->width, opr->height,
			ctx->srcbuf1);
	CHECK_BOUNDARY( opr->dx0,   opr->dy0,
			opr->width, opr->height,
			ctx->dstbuf);

	if (alpha0 == USE_SRC_ALPHA)
		imm &= ~0x1;
	if (alpha1 == USE_SRC_ALPHA)
		imm &= ~0x2;

	SETUP_ABLND( ctx->cmdbuf.cb_ptr, len,
			imm, 0, ctx->blend_op,
			opr->dx0, opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->s1x0, opr->s1y0,
			opr->width, opr->height,
			alpha0, alpha1);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_alpha_blend_single(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		unsigned int alpha0)
{
	int len = 0, imm = 1;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY( opr->s0x0,  opr->s0y0,
			opr->width, opr->height,
			ctx->srcbuf0);
	CHECK_BOUNDARY( opr->s1x0,  opr->s1y0,
			opr->width, opr->height,
			ctx->srcbuf1);
	CHECK_BOUNDARY( opr->dx0,   opr->dy0,
			opr->width, opr->height,
			ctx->dstbuf);

	if (alpha0 == USE_SRC_ALPHA)
		imm = 0;

	SETUP_ABLND( ctx->cmdbuf.cb_ptr, len,
			imm, 1, ctx->blend_op,
			opr->dx0, opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->s1x0, opr->s1y0,
			opr->width, opr->height,
			alpha0, 0);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_bias_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		unsigned int bias_factor)
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY( opr->s0x0, opr->s0y0,
			opr->width, opr->height,
			ctx->srcbuf0);
	CHECK_BOUNDARY( opr->dx0, opr->dy0,
			opr->width, opr->height,
			ctx->dstbuf);

	SETUP_BIAS(ctx->cmdbuf.cb_ptr, len,
			((bias_factor == USE_SRC_ALPHA) ? 0 : 1),
			ctx->bias_op,
			opr->dx0, opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->width, opr->height,
			bias_factor);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_raster_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		int raster_op)
{
	int len = 0;

	if (ctx == NULL) return -1;

	SETUP_RASTER(ctx->cmdbuf.cb_ptr, len,
			raster_op,
			opr->dx0, opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->s1x0, opr->s1y0,
			opr->width, opr->height);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_pattern_copy_mode1(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		int dw, int dh)
{
	int len = 0;

	if (ctx == NULL) return -1;

	SETUP_PATT(ctx->cmdbuf.cb_ptr, len, 0,
			opr->dx0, opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->width, opr->height,
			dw, dh);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_pattern_copy_mode2(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		int dw, int dh)
{
	int len = 0;

	if (ctx == NULL) return -1;

	SETUP_PATT(ctx->cmdbuf.cb_ptr, len, 1,
			opr->dx0, opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->width, opr->height,
			dw, dh);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

int m2d_rotate_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		GCU_ROTATION rot)
{
	int len = 0;

	if (ctx == NULL) return -1;

	CHECK_BOUNDARY( opr->s0x0, opr->s0y0,
			opr->width, opr->height,
			ctx->srcbuf0);

/* Only this check should be comment to support 180 degree rotation */
/*
	CHECK_BOUNDARY( opr->dx0, opr->dy0,
			opr->width, opr->height,
			ctx->dstbuf);
*/
	if(rot == GCU_ROTATE_180 || rot == GCU_ROTATE_0){
		CHECK_BOUNDARY( opr->dx0, opr->dy0,
				opr->width, opr->height,
				ctx->dstbuf);
	}else{
		CHECK_BOUNDARY( opr->dx0, opr->dy0,
				opr->height, opr->width,
				ctx->dstbuf);
	}

	if (rot > GCU_ROTATE_270)
		return -1;

	SETUP_ROTATE(ctx->cmdbuf.cb_ptr, len, rot,
			opr->dx0, opr->dy0,
			opr->s0x0, opr->s0y0,
			opr->width, opr->height);

	UPDATE_CMDBUF(ctx, len);
	return len;
}

