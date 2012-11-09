/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#undef 	LOG_TAG
#define LOG_TAG "COPYBIT_FRAMEWORK"
#include <utils/Log.h>
#include "copybit_inc.h"
#include "stats/sum_data.h"
//#include "gcu/gcu_copybit_inc.h"
#include "mmx/compsurf.h"
#include "linux/android_pmem.h"
#include "copybit_brn.h"

#include <linux/fb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

#ifdef __cplusplus
extern "C"{
#endif

#define PXAFB_MEM_FLUSH         _IOW('F', 0x85, unsigned int)
#define PXAFB_GET_FD_TYPE               _IOW('F', 0x86, unsigned int)
struct flush_mem_region {
	int fd;
	unsigned long base;
	unsigned long offset;
	unsigned long len;
};

#undef COPYBIT_WMMX_DEBUG
#undef TIME_DEBUG
#undef COPYBIT_USE_CASE_DEBUG
#undef COPYBIT_THROUGHPUT_DEBUG

#undef TIME_DEBUG

#define MAX_DIV_NUM 8
struct universal_2d_copybit {
	struct copybit_rect_t src_rect;
	struct copybit_rect_t dst_rect;
	struct copybit_image_t src_frame;
	struct copybit_image_t dst_frame;
};

//broncho add
static void set_rects_brn(
		copybit_context_t *dev,
		struct android_blit_req *e,
		const struct copybit_rect_t *dst,
		const struct copybit_rect_t *src,
		const struct copybit_rect_t *scissor);

//XXX: need keep consistent with private_handle_t(gralloc_priv.h)
typedef struct _private_handle_t 
{
    int version;        /* sizeof(native_handle_t) */
    int numFds;         /* number of file-descriptors at &data[0] */
    int numInts;        /* number of ints at &data[numFds] */

    // file-descriptors
    int     fd;
    // ints
    int     magic;
    int     flags;
    int     size;
    int     offset;

    // FIXME: the attributes below should be out-of-line
    int     base;
    int     lockState;
    int     writeOwner;
    int     pid;
}private_handle_t;

static native_handle_t* private_handle_create(void)
{
	private_handle_t* o = calloc(1, sizeof(private_handle_t));
	
	o->numFds = 1;
	o->numInts = 8;

	return (native_handle_t*)o;
};

static void private_handle_destroy(native_handle_t* o)
{
	free(o);

	return;
}

static void universal_2d_copybit_init(struct universal_2d_copybit o[MAX_DIV_NUM])
{
	int i = 0;
	for(i = 0; i < MAX_DIV_NUM; i++)
	{
		o[i].src_frame.handle = private_handle_create();
		o[i].dst_frame.handle = private_handle_create();
	}

	return;
}

static void universal_2d_copybit_deinit(struct universal_2d_copybit o[MAX_DIV_NUM])
{
	int i = 0;
	for(i = 0; i < MAX_DIV_NUM; i++)
	{
		private_handle_destroy(o[i].src_frame.handle);
		o[i].src_frame.handle = NULL;
		private_handle_destroy(o[i].dst_frame.handle);
		o[i].dst_frame.handle = NULL;
	}

	return;
}

static void copybit_image_init(struct copybit_image_t* img)
{
	img->handle = private_handle_create();

	return;
}

static void copybit_image_deinit(struct copybit_image_t* img)
{
	private_handle_destroy(img->handle);
	img->handle = NULL;

	return;
}

extern int set_mmx_rotate_parameter(
		copybit_context_t* ctx,
		IppRotateMode * rotateMode);

static struct hw_module_methods_t copybit_module_methods = {
	open: open_device
};

struct copybit_module_t HMI = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
		id: COPYBIT_HARDWARE_MODULE_ID,
		name: "Sample Copybit module",
		author: "Marvell",
		methods: &copybit_module_methods,
	}
};

typedef int (*p2DFunction)(
		copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *rect);

typedef struct _use_case {
	int eSrcFormat;
	int eDstFormat;
	int bResize;
	int bRotate;
	int bGlobalAlpha;
	int bPixelAlpha;
	int iCandidate;
	p2DFunction  fAccelerateFunction;
} UseCase;

#define WVGA_RGBA8888_LEN 800*480*4
static int wmmx_pmem_fd = 0;
unsigned char *wmmx_pmem_buf = NULL;
static int open_wmmx_pmem(unsigned char **base)
{
	int ret = -EINVAL;
	if (wmmx_pmem_fd <= 0) {
		wmmx_pmem_fd = open("/dev/pmem_wmmx", O_RDWR);
		if (wmmx_pmem_fd < 0) {
			LOGE("In open_wmmx_pmem: Failed to open /dev/pmem_wmmx\n");
			goto err1;
		}
		wmmx_pmem_buf = (unsigned char *)mmap(0, WVGA_RGBA8888_LEN,
				PROT_READ|PROT_WRITE, MAP_SHARED, wmmx_pmem_fd, 0);
		if (wmmx_pmem_buf == MAP_FAILED) {
			LOGE("In open_wmmx_pmem: Failed to mmap wmmx pmem\n");
			goto err2;
		}
	}
	*base = wmmx_pmem_buf;
	return wmmx_pmem_fd;
err2:
	close(wmmx_pmem_fd);
	wmmx_pmem_fd = 0;
	wmmx_pmem_buf = NULL;
err1:
	base = NULL;
	return ret;
}

static void close_wmmx_pmem()
{
	if (wmmx_pmem_fd > 0) {
		munmap(wmmx_pmem_buf, WVGA_RGBA8888_LEN);
		close(wmmx_pmem_fd);
		wmmx_pmem_buf = NULL;
		wmmx_pmem_fd = 0;
	}
}

inline int copybit_get_step_size(int format)
{
	int step_size;
	switch(format) {
		case COPYBIT_FORMAT_RGB_565:
			step_size = 2;
			break;
		case COPYBIT_FORMAT_RGBA_8888:
		case COPYBIT_FORMAT_BGRA_8888:
			step_size = 4;
			break;
		/*TODO: other pixel storage*/
		default:
			return -ERROR_UNSUPPORT_FORMAT;
	}
	return step_size;
}

static inline void copy_clip_rect(struct copybit_rect_t *dst_rect, struct android_rect *src_rect)
{
	dst_rect->l = src_rect->x;
	dst_rect->t = src_rect->y;
	dst_rect->r = src_rect->x + src_rect->w;
	dst_rect->b = src_rect->y + src_rect->h;
}


/*
 * src_fmt    dst_fmt    blending    resize    rotate    solution
 * RGB565     RGB565     copy         ---       ---      2D GCU
 */
static inline int rgb565_rgb565_copy_gcu(struct copybit_context_t *ctx,
				struct copybit_image_t const *dst,
				struct copybit_image_t const *src,
				struct copybit_rect_t const *dst_rect,
				struct copybit_rect_t const *src_rect,
				struct copybit_rect_t const *rect)
{

	return 0;
}

/*
 * src_fmt    dst_fmt    blending    resize    rotate    solution
 * RGB5656    RGB565                 Resize       ---      WMMX
 */
static inline int rgb565_rgb565_resize_wmmx(struct copybit_context_t *ctx,
				struct copybit_image_t const *dst,
				struct copybit_image_t const *src,
				struct copybit_rect_t const *dst_rect,
				struct copybit_rect_t const *src_rect,
				struct copybit_rect_t const *rect)
{
	return marvell_mmx_copybit(ctx, dst, src, dst_rect, src_rect, rect, ippBlendCopy, 255);
}

/*
 * src_fmt    dst_fmt    blending    resize    rotate    solution
 * RGBA8888   RGB565     copy         ---       ---      WMMX
 */
static inline int rgba8888_rgb565_copy_wmmx(struct copybit_context_t *ctx,
				struct copybit_image_t const *dst,
				struct copybit_image_t const *src,
				struct copybit_rect_t const *dst_rect,
				struct copybit_rect_t const *src_rect,
				struct copybit_rect_t const *rect)
{
	return marvell_mmx_copybit(ctx, dst, src, dst_rect, src_rect, rect, ippBlendCopy, 255);
}


/*
 * src_fmt    dst_fmt    blending    resize    rotate    solution
 * RGB565     RGB565     copy         ---      90R/90L    WMMX
 */
static inline int rgb565_rgb565_copy_rotate_wmmx(struct copybit_context_t *ctx,
				struct copybit_image_t const *dst,
				struct copybit_image_t const *src,
				struct copybit_rect_t const *dst_rect,
				struct copybit_rect_t const *src_rect,
				struct copybit_rect_t const *rect)
{
	return marvell_mmx_copybit(ctx, dst, src, dst_rect, src_rect, rect, ippBlendCopy, 255);
}

/*
 * src_fmt    dst_fmt    blending    resize    rotate    solution
 * RGB565     RGB565     copy        Resize    90R/90L    WMMX
 */
static inline int rgb565_rgb565_resize_rotate_wmmx(struct copybit_context_t *ctx,
				struct copybit_image_t const *dst,
				struct copybit_image_t const *src,
				struct copybit_rect_t const *dst_rect,
				struct copybit_rect_t const *src_rect,
				struct copybit_rect_t const *rect)
{
	/* Rotate first, then resize */
	struct copybit_image_t tmp_dst_image;
	struct copybit_rect_t tmp_dst_rect;
	struct copybit_rect_t tmp_src_rect;
	struct copybit_rect_t clip_rect;
	struct android_blit_req req;
	int trans, ret = 0;

	set_rects_brn(ctx, &req, dst_rect, src_rect, rect);
	tmp_dst_rect.l = 0;
	tmp_dst_rect.t = 0;
	if (ctx->mTransformation == COPYBIT_TRANSFORM_ROT_90 ||
			ctx->mTransformation == COPYBIT_TRANSFORM_ROT_270) {
		tmp_dst_rect.r = req.src_rect.h;
		tmp_dst_rect.b = req.src_rect.w;
	} else {
		tmp_dst_rect.r = req.src_rect.w;
		tmp_dst_rect.b = req.src_rect.h;
	}
	copybit_image_init(&tmp_dst_image);

	tmp_dst_image.w = tmp_dst_rect.r;
	tmp_dst_image.h = tmp_dst_rect.b;
	tmp_dst_image.format = src->format;
	IMAGE_OFFSET(&tmp_dst_image) = 0;
	IMAGE_BASE(&tmp_dst_image) = ctx->wmmx_pmem_buf;
	IMAGE_FD(&tmp_dst_image)= ctx->wmmx_pmem_fd;
	tmp_src_rect.l = req.src_rect.x;
	tmp_src_rect.t = req.src_rect.y;
	tmp_src_rect.r = req.src_rect.x + req.src_rect.w;
	tmp_src_rect.b = req.src_rect.y + req.src_rect.h;

	if (marvell_mmx_copybit(ctx,
				&tmp_dst_image,
				src,
				&tmp_dst_rect,
				&tmp_src_rect,
				&tmp_dst_rect,
				ippBlendCopy,
				255) < 0) {

		LOGE("%s: marvell_mmx_copybit rotate error", __func__);
		return -EINVAL;
	}
	/* Resize */
	trans = ctx->mTransformation;
	ctx->mTransformation = 0;
	copy_clip_rect(&clip_rect, &(req.dst_rect));
	ret = marvell_mmx_copybit(ctx,
			dst, &tmp_dst_image,
			&clip_rect, &tmp_dst_rect,
			&clip_rect, ippBlendCopy, 255);
	if (ret < 0) {
		LOGE("%s: marvell_mmx_copybit resize error", __func__);
		ctx->mTransformation = trans;
		return -EINVAL;
	}
	ctx->mTransformation = trans;
	copybit_image_deinit(&tmp_dst_image);

	return ret;
}

static inline void copy_image_info(struct copybit_image_t *dst,
					struct copybit_image_t const *src);
/*
 * src_fmt    dst_fmt    blending    resize    rotate    solution
 * RGBA8888     RGB565     copy         ---      90R/90L   WMMX
 */
static inline int rgba8888_rgb565_copy_rotate_wmmx(struct copybit_context_t *ctx,
				struct copybit_image_t const *dst,
				struct copybit_image_t const *src,
				struct copybit_rect_t const *dst_rect,
				struct copybit_rect_t const *src_rect,
				struct copybit_rect_t const *rect)
{
	int ret;
	struct copybit_image_t dst_frame;

	copybit_image_init(&dst_frame);
	copy_image_info(&dst_frame, dst);
	IMAGE_BASE(&dst_frame) = wmmx_pmem_buf;
	ret = rgba8888_rgb565_copy_wmmx(ctx, &dst_frame, src,
					dst_rect, src_rect, rect);
	if (ret < 0) {
		LOGE("%s: rgba8888_rgb565_copy_wmmx error.", __func__);
		return -EINVAL;
	}

	ret = rgb565_rgb565_copy_rotate_wmmx(ctx, dst, &dst_frame,
					dst_rect, src_rect, rect);

	if (ret < 0) {
		LOGE("%s: rgb565_rgb565_copy_rotate_wmmx error.", __func__);
		return -EINVAL;
	}
	copybit_image_deinit(&dst_frame);

	return ret;
}

static inline int SCALE(int a, int mul, int div)
{
	return (mul == div ? a : (a * mul / div));
}

static inline void copy_image_info(struct copybit_image_t *dst, struct copybit_image_t const *src)
{
	dst->w = src->w;
	dst->h = src->h;
	dst->format = src->format;
	IMAGE_BASE(dst) = IMAGE_BASE(src);
	IMAGE_OFFSET(dst) = IMAGE_OFFSET(src);
	IMAGE_FD(dst) = IMAGE_FD(src);
}

static inline void copy_rect_info(struct copybit_rect_t *dst_rect, struct copybit_rect_t const *src_rect)
{
	dst_rect->l = src_rect->l;
	dst_rect->r = src_rect->r;
	dst_rect->t = src_rect->t;
	dst_rect->b = src_rect->b;
}

static inline void copy_Surface_info(IppSurface *dst, struct copybit_image_t const *src,uint32_t format)
{
	dst->width = src->w;
	dst->height = src->h;
	dst->pBuf = IMAGE_BASE(src);
	dst->stride = copybit_get_step_size(format) * src->w;
	dst->clrFormat = ippColorFmtARGB8888;
}

/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGB565     RGB565     Global Alpha   Resize    ---       Hybrid
 */
int rgb565_rgb565_resize_galpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return 0;
}

/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGB565     RGB565     Global Alpha   Resize    90R/90L   Hybrid
 */
int rgb565_rgb565_resize_rotate_galpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return 0;
}



/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGB565     RGB565     Global Alpha   ---	  ---       Hybrid
 */
inline int rgb565_rgb565_galpha_gcu(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	int ret = 0;
	return 0;
}

/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGB565     RGB565     Global Alpha   ---       90R/90L   Hybrid
 */
inline int rgb565_rgb565_rotate_galpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	/* Rotate first, then alpha blending by GCU */
	return 0;
}


/*
 * src_fmt    dst_fmt    blending	       resize    rotate    solution
 * RGBA8888   RGB565     Global/Pixel Alpha    Resize    ---       Hybrid
 */
int rgba8888_rgb565_resize_galpha_palpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return 0;
}


/*
 * src_fmt    dst_fmt    blending	       resize    rotate    solution
 * RGBA8888   RGB565     Global/Pixel Alpha    Resize    90R/90L   Hybrid
 */
int rgba8888_rgb565_resize_rotate_galpha_palpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return 0;
}

/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGBA8888   RGB565     Pixel Alpha    ---       ---       Hybrid
 */
inline int rgba8888_rgb565_palpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return rgba8888_rgb565_resize_galpha_palpha_hybrid(ctx, dst, src,
							dst_rect, src_rect, prect);
}

/*
 * src_fmt    dst_fmt    blending	       resize    rotate    solution
 * RGBA8888   RGB565     Global/Pixel Alpha    ---       ---       Hybrid
 */
inline int rgba8888_rgb565_galpha_palpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return rgba8888_rgb565_resize_galpha_palpha_hybrid(ctx, dst, src,
							dst_rect, src_rect, prect);
}


/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGBA8888   RGB565     Pixel Alpha    Resize    ---       Hybrid
 */
inline int rgba8888_rgb565_resize_palpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return rgba8888_rgb565_resize_galpha_palpha_hybrid(ctx, dst, src,
							dst_rect, src_rect, prect);
}

/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGBA8888   RGB565     Pixel Alpha    ---       ---       WMMX
 */
inline int rgba8888_rgb565_palpha_wmmx(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return marvell_mmx_copybit(ctx, dst, src, dst_rect, src_rect,
					prect, ippBlendSrcOvr, ctx->mPlaneAlpha);
}

/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGBA8888   RGB565     Pixel Alpha    Resize    ---       WMMX
 */
inline int rgba8888_rgb565_resize_palpha_wmmx(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return marvell_mmx_copybit(ctx, dst, src, dst_rect, src_rect,
					prect, ippBlendSrcOvr, ctx->mPlaneAlpha);
}

/*
 * src_fmt    dst_fmt    blending	       resize    rotate    solution
 * RGBA8888   RGB565     Global/Pixel Alpha    Resize    ---       WMMX
 */
inline int rgba8888_rgb565_resize_galpha_palpha_wmmx(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	int trans, ret;

	trans = ctx->mTransformation;
	ctx->mTransformation = 0;

	if ((ret = marvell_mmx_copybit(ctx, dst, src, dst_rect,
				src_rect, prect, ippBlendSrcOvr, ctx->mPlaneAlpha)) < 0) {
		ctx->mTransformation = trans;
		LOGE("%s: marvell_mmx_copybit error", __func__);
		return -EINVAL;
	}

	ctx->mTransformation = trans;

	return ret;
}

/*
 * src_fmt    dst_fmt    blending	       resize    rotate    solution
 * RGBA8888   RGB565     Pixel/Global Alpha    ---       ---       WMMX
 */
inline int rgba8888_rgb565_galpha_palpha_wmmx(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return rgba8888_rgb565_resize_galpha_palpha_wmmx(ctx, dst, src,
							dst_rect, src_rect, prect);
}

/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGBA8888   RGB565     Pixel Alpha    ---       90R/90L   Hybrid
 */
inline int rgba8888_rgb565_rotate_palpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return rgba8888_rgb565_resize_rotate_galpha_palpha_hybrid(ctx, dst, src,
							dst_rect, src_rect, prect);
}

/*
 * src_fmt    dst_fmt    blending	       resize    rotate    solution
 * RGBA8888   RGB565     Global/Pixel Alpha    ---       90R/90L   Hybrid
 */
inline int rgba8888_rgb565_rotate_galpha_palpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return rgba8888_rgb565_resize_rotate_galpha_palpha_hybrid(ctx, dst, src,
							dst_rect, src_rect, prect);
}


/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGBA8888   RGB565     Pixel Alpha    Resize    90R/90L   Hybrid
 */
inline int rgba8888_rgb565_resize_rotate_palpha_hybrid(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return rgba8888_rgb565_resize_rotate_galpha_palpha_hybrid(ctx, dst, src,
							dst_rect, src_rect, prect);
}


/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGBA8888   RGB565     Pixel Alpha    Resize    90R/90L   WMMX
 */
inline int rgba8888_rgb565_resize_rotate_palpha_wmmx(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	int ret;
	struct copybit_image_t tmp_dst_image;
	struct copybit_rect_t tmp_dst_rect;
	struct copybit_rect_t tmp_src_rect;
	struct copybit_rect_t clip_rect;
	struct android_blit_req req;
	int trans;

	/* Rotate First */
	set_rects_brn(ctx, &req, dst_rect, src_rect, prect);
	tmp_dst_rect.l = 0;
	tmp_dst_rect.t = 0;
	if (ctx->mTransformation == COPYBIT_TRANSFORM_ROT_90 ||
			ctx->mTransformation == COPYBIT_TRANSFORM_ROT_270) {
		tmp_dst_rect.r = req.src_rect.h;
		tmp_dst_rect.b = req.src_rect.w;
	} else {
		tmp_dst_rect.r = req.src_rect.w;
		tmp_dst_rect.b = req.src_rect.h;
	}
	copybit_image_init(&tmp_dst_image);
	tmp_dst_image.w = tmp_dst_rect.r;
	tmp_dst_image.h = tmp_dst_rect.b;
	IMAGE_OFFSET(&tmp_dst_image) = 0;
	IMAGE_BASE(&tmp_dst_image) = wmmx_pmem_buf;
	tmp_dst_image.format = src->format;
	tmp_src_rect.l = req.src_rect.x;
	tmp_src_rect.t = req.src_rect.y;
	tmp_src_rect.r = req.src_rect.x + req.src_rect.w;
	tmp_src_rect.b = req.src_rect.y + req.src_rect.h;

	if (marvell_mmx_copybit(ctx,
				&tmp_dst_image,
				src,
				&tmp_dst_rect,
				&tmp_src_rect,
				&tmp_dst_rect,
				ippBlendCopy,
				255) < 0) {

		LOGE("%s: marvell_mmx_copybit error", __func__);
		copybit_image_deinit(&tmp_dst_image);
		return -EINVAL;
	}

	/* Resize and Pixel alpha blending */
	trans = ctx->mTransformation;
	ctx->mTransformation = 0;
	copy_clip_rect(&clip_rect, &(req.dst_rect));
	ret = rgba8888_rgb565_resize_palpha_wmmx(ctx, dst, &tmp_dst_image,
					&clip_rect, &tmp_dst_rect, &clip_rect);
	if (ret < 0) {
		ctx->mTransformation = trans;
		LOGE("%s: rgba8888_rgb565_resize_palpha_wmmx error", __func__);
		copybit_image_deinit(&tmp_dst_image);
		return -EINVAL;
	}

	ctx->mTransformation = trans;
	copybit_image_deinit(&tmp_dst_image);

	return 0;
}


/*
 * src_fmt    dst_fmt    blending	resize    rotate    solution
 * RGBA8888   RGB565     Pixel Alpha    ---       90R/90L   WMMX
 */
inline int rgba8888_rgb565_rotate_palpha_wmmx(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return rgba8888_rgb565_resize_rotate_palpha_wmmx(ctx, dst, src,
					dst_rect, src_rect, prect);
}

/*
 * src_fmt    dst_fmt    blending	       resize    rotate    solution
 * RGBA8888   RGB565     Global/Pixel Alpha    ---       90R/90L   WMMX
 */
inline int rgba8888_rgb565_rotate_galpha_palpha_wmmx(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return rgba8888_rgb565_resize_rotate_palpha_wmmx(ctx, dst, src,
					dst_rect, src_rect, prect);
}

/*
 * src_fmt    dst_fmt    blending	      resize    rotate    solution
 * RGBA8888   RGB565     Global/Pixel Alpha   Resize    90R/90L   WMMX
 */
inline int rgba8888_rgb565_resize_rotate_galpha_palpha_wmmx(copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *prect)
{
	return rgba8888_rgb565_resize_rotate_palpha_wmmx(ctx, dst, src,
					dst_rect, src_rect, prect);
}




#define INIT_RGB_RGB_PATH(src_fmt, dst_fmt, res, rot, galpha, palpha, cand, func) \
	{COPYBIT_FORMAT_RGB_##src_fmt, COPYBIT_FORMAT_RGB_##dst_fmt, res, rot, galpha, palpha, \
		cand, rgb##src_fmt##_rgb##dst_fmt##func}

#define INIT_RGBA_RGB_PATH(src_fmt, dst_fmt, res, rot, galpha, palpha, cand, func) \
	{COPYBIT_FORMAT_RGBA_##src_fmt, COPYBIT_FORMAT_RGB_##dst_fmt, res, rot, galpha, palpha, \
		cand, rgba##src_fmt##_rgb##dst_fmt##func}

static const UseCase use_case [] = {

	/*** Solution One ***/

	/* Screen Sliding */
	//INIT_RGB_RGB_PATH(565, 565, 0, 0, 0, 0, 0, _copy_gcu),
	INIT_RGB_RGB_PATH(565, 565, 1, 0, 0, 0, 0, _resize_wmmx),
	INIT_RGBA_RGB_PATH(8888, 565, 0, 0, 0, 0, 0, _copy_wmmx),

	/* Screen Sliding on rotated screen */
	INIT_RGB_RGB_PATH(565, 565, 0, 1, 0, 0, 0, _copy_rotate_wmmx),
	INIT_RGB_RGB_PATH(565, 565, 1, 1, 0, 0, 0, _resize_rotate_wmmx),
	INIT_RGBA_RGB_PATH(8888, 565, 0, 1, 0, 0, 0, _copy_rotate_wmmx),

	/* App Popup */
	//INIT_RGB_RGB_PATH(565, 565, 0, 0, 1, 0, 0, _galpha_gcu),
	//INIT_RGB_RGB_PATH(565, 565, 1, 0, 1, 0, 0, _resize_galpha_hybrid),
	INIT_RGBA_RGB_PATH(8888, 565, 0, 0, 0, 1, 0, _palpha_wmmx),
	INIT_RGBA_RGB_PATH(8888, 565, 0, 0, 1, 1, 0, _galpha_palpha_wmmx),
	INIT_RGBA_RGB_PATH(8888, 565, 1, 0, 0, 1, 0, _resize_palpha_wmmx),
	INIT_RGBA_RGB_PATH(8888, 565, 1, 0, 1, 1, 0, _resize_galpha_palpha_wmmx),

	/* App Popup on rotated screen */
	//INIT_RGB_RGB_PATH(565, 565, 0, 1, 1, 0, 0, _rotate_galpha_hybrid),
	//INIT_RGB_RGB_PATH(565, 565, 1, 1, 1, 0, 0, _resize_rotate_galpha_hybrid),
	//INIT_RGBA_RGB_PATH(8888, 565, 0, 1, 0, 1, 0, _rotate_palpha_hybrid),
	INIT_RGBA_RGB_PATH(8888, 565, 0, 1, 1, 1, 0, _rotate_galpha_palpha_wmmx),
	INIT_RGBA_RGB_PATH(8888, 565, 1, 1, 0, 1, 0, _resize_rotate_palpha_wmmx),
	INIT_RGBA_RGB_PATH(8888, 565, 1, 1, 1, 1, 0, _resize_rotate_galpha_palpha_wmmx),

#if 0
	/* You can change _iCandidate_ and _fAccelerateFunction_ parameter to user different solution */

	/*** Example Two ***/
	INIT_RGBA_RGB_PATH(8888, 565, 0, 0, 0, 1, 1, _palpha_hybrid),
	INIT_RGBA_RGB_PATH(8888, 565, 0, 0, 1, 1, 1, _galpha_palpha_hybrid),
	INIT_RGBA_RGB_PATH(8888, 565, 1, 0, 0, 1, 1, _resize_palpha_hybrid),
	INIT_RGBA_RGB_PATH(8888, 565, 1, 0, 1, 1, 1, _resize_galpha_palpha_hybrid),
	/* App Popup on rotated screen */
	INIT_RGBA_RGB_PATH(8888, 565, 0, 1, 0, 1, 1, _rotate_palpha_wmmx),
	INIT_RGBA_RGB_PATH(8888, 565, 0, 1, 1, 1, 1, _rotate_galpha_palpha_hybrid),
	INIT_RGBA_RGB_PATH(8888, 565, 1, 1, 0, 1, 1, _resize_rotate_palpha_hybrid),
	INIT_RGBA_RGB_PATH(8888, 565, 1, 1, 1, 1, 1, _resize_rotate_galpha_palpha_hybrid),

	.......... /* other paths init */


#endif
};
#define USE_CASE_NUM	(sizeof(use_case) / sizeof(use_case[0]))

static int open_device(
		const struct hw_module_t* module,
		const char* name,
		struct hw_device_t** device)
{
	int status = -EINVAL;

	copybit_context_t *pContext = (copybit_context_t*)malloc(sizeof(copybit_context_t));
	if (NULL == pContext) {
		LOGE("Failed to malloc memory!");
		goto err1;
	}
	memset(pContext, 0, sizeof(copybit_context_t));

	/* broncho del
	if (m2d_copybit_open(&(pContext->context)) < 0) {
		LOGE("Failed to open m2d gcu!");
		free(pContext);
		goto err2;
	}
	*/

	pContext->mFD = open("/dev/graphics/fb0", O_RDWR, 0);
	if (pContext->mFD < 0) {
		LOGE("Failed to open frame buffer\n");
		goto err2;
	}

	pContext->wmmx_pmem_fd = open_wmmx_pmem(&(pContext->wmmx_pmem_buf));
	if (pContext->wmmx_pmem_fd < 0) {
		LOGE("Failed to open wmmx pmem\n");
		goto err4;
	}

	// initialize the procs
	pContext->device.common.tag = HARDWARE_DEVICE_TAG;
	pContext->device.common.version = 0;
	pContext->device.common.module = (struct hw_module_t*)(module);
	pContext->device.common.close = close_device;

	pContext->device.set_parameter = copybit_set_parameter;
	pContext->device.get = copybit_get_parameter;
	pContext->device.blit = copybit_blit;
	pContext->device.stretch = copybit_stretch;

	pContext->mRotationDegree = 0;
	pContext->mPlaneAlpha = 0xFF;
	pContext->mTransformation = 0;
	pContext->mTransformationLast = 0;
	pContext->mDitherEnabled = COPYBIT_DISABLE;
	pContext->mBlurEnabled = COPYBIT_DISABLE;

	*device = &pContext->device.common;

	LOGE("%s: Open copybit device succussfully\n", __func__);

	return 0;
err4:
	close(pContext->mFD);
	/*broncho del
err3:
	m2d_copybit_close(&(pContext->context));
	*/
err2:
	free(pContext);
err1:
	return status;
}

static int close_device(struct hw_device_t* dev)
{
	copybit_context_t* ctx = (copybit_context_t*) dev;

	if(NULL != dev) {
		close_wmmx_pmem();
		close(ctx->mFD);
		// broncho del
		//m2d_copybit_close(&(ctx->context));
		free(dev);
	}

	return 0 ;
}


static int copybit_set_parameter(
		struct copybit_device_t *dev,
		int name,
		int value)
{
	copybit_context_t *ctx = (copybit_context_t *)dev;
	int status = -EINVAL;

	if(dev == NULL) {
		LOGE("copybit device is NULL!\n");
		return status;
	}

	switch(name) {
		case COPYBIT_ROTATION_DEG:
			LOGW("COPYBIT_ROTATION_DEG is not supported.\n");
			break;
		case COPYBIT_PLANE_ALPHA:
			value = value > 0xFF ? 0xFF : value;
			value = value < 0 ? 0 : value;
			ctx->mPlaneAlpha = value;
			status = NO_ERROR;
			break;
		case COPYBIT_DITHER:
			// FIXME: GC300 current doesn't support DITHER
			if(value ==COPYBIT_DISABLE || value == COPYBIT_ENABLE) {
				ctx->mDitherEnabled = value;
				status = NO_ERROR;
			}
			else {
				LOGW("Invalid dither enabling flag: %d\n", value);
			}
			break;
		case COPYBIT_BLUR:
			if(value ==COPYBIT_DISABLE || value == COPYBIT_ENABLE) {
				ctx->mBlurEnabled = value;
				status = NO_ERROR;
			}
			else {
				LOGW("Invalid dither enabling flag: %d\n", value);
			}
			break;
		case COPYBIT_TRANSFORM:
			switch(value) {
				case COPYBIT_TRANSFORM_FLIP_H:
				case COPYBIT_TRANSFORM_FLIP_V:
				case COPYBIT_TRANSFORM_ROT_90:
				case COPYBIT_TRANSFORM_ROT_180:
				case COPYBIT_TRANSFORM_ROT_270:
				case 0:
					ctx->mTransformation = value;
					break;
				default:
					LOGW("Invalid transformation value: %d\n", value);
					break;
			}
			break;
		default:
			LOGW("Invalid parameter name: %d", name);
			break;
	}

	return status;
}

static int copybit_get_parameter(
		struct copybit_device_t *dev,
		int name)
{
	int rel = -EINVAL;
	switch(name) {
		case COPYBIT_MINIFICATION_LIMIT:
			rel = 16;
			break;
		case COPYBIT_MAGNIFICATION_LIMIT:
			rel = 4;
			break ;
		case COPYBIT_SCALING_FRAC_BITS:
			rel = 16;
			break ;
		case COPYBIT_ROTATION_STEP_DEG:
			rel = 90;
			break ;
		default :
			LOGE("Invalid id [%d] for static informations of the hardware\n", name);
			break;
	}
	return rel;
}

static int copybit_stretch(
		struct copybit_device_t *dev,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_region_t const *region)
{
	LOGI("%s: src_base=%p dst_base=%p w=%d h=%d\n", 
		__func__, IMAGE_BASE(src), IMAGE_BASE(dst), dst->w, dst->h);

	return marvell_copybit(dev, dst, src, dst_rect, src_rect, region);
}

static int copybit_blit(
		struct copybit_device_t *dev,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_region_t const *region)
{
	struct copybit_rect_t bounds = {0, 0, dst->w, dst->h};
	
	LOGI("%s: src_base=%p dst_base=%p w=%d h=%d\n", 
		__func__, IMAGE_BASE(src), IMAGE_BASE(dst), dst->w, dst->h);

	return marvell_copybit(dev, dst, src, &bounds, &bounds, region);
}

static int marvell_copybit(
		struct copybit_device_t *dev,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_region_t const *region)
{
	int status = NO_ERROR;
	struct copybit_rect_t rect;
	struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
	struct copybit_region_t * pRegion = (struct copybit_region_t *)region;
	p2DFunction accelerate_function = NULL;
	uint32_t i;
	int bResize = 0, bRotate = 0, bPixelAlpha = 0, bGlobalAlpha = 0, bBlur = 0, bDither = 0, bCopy = 0;
	int candidate_index = 0;
	int iSrcW = src_rect->r - src_rect->l;
	int iSrcH = src_rect->b - src_rect->t;
	int iDstW = dst_rect->r - dst_rect->l;
	int iDstH = dst_rect->b - dst_rect->t;
	struct flush_mem_region l_region;

	if (dev == NULL) {
		LOGE("copybit device is NULL!\n");
		return -1;
	}

	bRotate = ctx->mTransformation != 0;
	bResize = (bRotate && (iSrcW != iDstH || iSrcH != iDstW)) || (!bRotate && (iSrcW != iDstW || iSrcH != iDstH));
	bPixelAlpha = (src->format == COPYBIT_FORMAT_RGBA_8888 ||
			src->format == COPYBIT_FORMAT_BGRA_8888 ||
			src->format == COPYBIT_FORMAT_RGBA_5551 ||
			src->format == COPYBIT_FORMAT_RGBA_4444);
	bGlobalAlpha = ctx->mPlaneAlpha != 255;

	bDither = ctx->mDitherEnabled;
	bBlur = ctx->mBlurEnabled;

	bCopy = !bResize && !bRotate && !bPixelAlpha && !bGlobalAlpha;

	/* Parse the src format, dst format, blending, resize, rotate, candidate option */
	for (i = 0; i < USE_CASE_NUM; i++) {
		if (use_case[i].eSrcFormat == src->format &&
				use_case[i].eDstFormat == dst->format &&
				!(use_case[i].bResize ^ bResize) &&
				!(use_case[i].bRotate ^ bRotate) &&
				!(use_case[i].bGlobalAlpha ^ bGlobalAlpha) &&
				!(use_case[i].bPixelAlpha ^ bPixelAlpha) &&
				(candidate_index == use_case[i].iCandidate))
		{
#ifdef COPYBIT_USE_CASE_DEBUG
			LOGI("%s: i=%d srcf(%d), dstf(%d), res(%d), rot(%d), ga(%d), pa(%d), ic(%d)\n",
					__func__,
					i,
					src->format,
					dst->format,
					bResize,
					bRotate,
					bGlobalAlpha,
					bPixelAlpha,
					candidate_index);
#endif
			accelerate_function = use_case[i].fAccelerateFunction;
			if (accelerate_function == NULL) {
				LOGE("%s: accelerate_function is NULL, src format(%d), dst format(%d)\n",
						__func__,
						src->format,
						dst->format);
				return -EINVAL;
			}
			break;
		}
	}
	if (i == USE_CASE_NUM) {
		LOGE("%s: unsupport case: srcf(%d), dstf(%d), res(%d), rot(%d), ga(%d), pa(%d), ic(%d)\n",
				__func__,
				src->format,
				dst->format,
				bResize,
				bRotate,
				bGlobalAlpha,
				bPixelAlpha,
				candidate_index);
		return -EINVAL;
	}

	while (pRegion->next(pRegion, &rect))
	{
#ifdef COPYBIT_THROUGHPUT_DEBUG
		log_copybit_start();
#endif
		status = accelerate_function(ctx, dst, src, dst_rect, src_rect, &rect);
#ifdef COPYBIT_THROUGHPUT_DEBUG
		log_copybit_end(ctx, src, src_rect, dst, dst_rect, &rect);
#endif
	}
	if (status == NO_ERROR) {
		l_region.fd = IMAGE_FD(dst);
		l_region.base = (unsigned long)IMAGE_BASE(dst);
		l_region.offset = IMAGE_OFFSET(dst);
		l_region.len = copybit_get_step_size(dst->format) * dst->w * dst->h;
		status = ioctl(ctx->mFD, PXAFB_MEM_FLUSH, &l_region);
		if (status == -1) {
			LOGE("%s: Flush framebuffer error\n", __func__);
			return -EINVAL;
		}
	}
	return status;
}

/**===================================================**/
// broncho add
/** min of int a, b */
static inline int min(int a, int b) {
	return (a<b) ? a : b;
}

/** max of int a, b */
static inline int max(int a, int b) {
	return (a>b) ? a : b;
}

/** scale each parameter by mul/div. Assume div isn't 0 */
static inline void MULDIV(uint32_t *a, uint32_t *b, int mul, int div) {
	if (mul != div) {
		*a = (mul * *a) / div;
		*b = (mul * *b) / div;
	}
}

/** Determine the intersection of lhs & rhs store in out */
static void intersect(
		struct copybit_rect_t *out,
		const struct copybit_rect_t *lhs,
		const struct copybit_rect_t *rhs)
{
	out->l = max(lhs->l, rhs->l);
	out->t = max(lhs->t, rhs->t);
	out->r = min(lhs->r, rhs->r);
	out->b = min(lhs->b, rhs->b);
}

/** setup rectangles */
static void set_rects_brn(
		copybit_context_t *dev,
		struct android_blit_req *e,
		const struct copybit_rect_t *dst,
		const struct copybit_rect_t *src,
		const struct copybit_rect_t *scissor)
{
	struct copybit_rect_t clip;
	intersect(&clip, scissor, dst);

	e->dst_rect.x  = clip.l;
	e->dst_rect.y  = clip.t;
	e->dst_rect.w  = clip.r - clip.l;
	e->dst_rect.h  = clip.b - clip.t;

	uint32_t W, H;
	if (dev->mTransformation & COPYBIT_TRANSFORM_ROT_90) {
		e->src_rect.x = (clip.t - dst->t) + src->t;
		e->src_rect.y = (dst->r - clip.r) + src->l;
		e->src_rect.w = (clip.b - clip.t);
		e->src_rect.h = (clip.r - clip.l);
		W = dst->b - dst->t;
		H = dst->r - dst->l;
	}
	else
	{
		e->src_rect.x  = (clip.l - dst->l) + src->l;
		e->src_rect.y  = (clip.t - dst->t) + src->t;
		e->src_rect.w  = (clip.r - clip.l);
		e->src_rect.h  = (clip.b - clip.t);
		W = dst->r - dst->l;
		H = dst->b - dst->t;
	}

	MULDIV(&e->src_rect.x, &e->src_rect.w, src->r - src->l, W);
	MULDIV(&e->src_rect.y, &e->src_rect.h, src->b - src->t, H);

	if (dev->mTransformation & COPYBIT_TRANSFORM_FLIP_V)
	{
		e->src_rect.y = e->src.height - (e->src_rect.y + e->src_rect.h);
	}

	if (dev->mTransformation & COPYBIT_TRANSFORM_FLIP_H)
	{
		e->src_rect.x = e->src.width  - (e->src_rect.x + e->src_rect.w);
	}
}

#ifdef __cplusplus
}
#endif
