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
#define LOG_TAG "COPYBIT_MMX"
#include <utils/Log.h>

#include "../copybit_inc.h"
#include "compsurf.h"

#ifdef __cplusplus
extern "C"{
#endif

#undef COPYBIT_WMMXLIB_DEBUG

static int set_mmx_surface_parameter(
		struct copybit_image_t const *img,
		IppSurface * ippSurface,
		struct copybit_rect_t const * rect,
		IppRect * ipprect)
{
	ippSurface->pBuf = (unsigned char*)IMAGE_BASE(img) + IMAGE_OFFSET(img);
	ippSurface->width = img->w;
	ippSurface->height = img->h;

	switch (img->format)
	{
	case COPYBIT_FORMAT_RGBA_8888:
		ippSurface->clrFormat = ippColorFmtABGR8888;
		ippSurface->stride = img->w * 4;
		break;
	case COPYBIT_FORMAT_BGRA_8888:
		ippSurface->clrFormat = ippColorFmtARGB8888;
		ippSurface->stride = img->w * 4;
		break;
	case COPYBIT_FORMAT_RGB_565:
		ippSurface->clrFormat = ippColorFmtRGB565;
		ippSurface->stride = img->w * 2;
		break;
	default:
		ippSurface->clrFormat = ippColorFmtCount;
		ippSurface->stride = 0;
		break;
	}

	ipprect->left = rect->l;
	ipprect->top = rect->t;
	ipprect->right = rect->r;
	ipprect->bottom = rect->b;

	return 0;
}



int set_mmx_rotate_parameter(
		copybit_context_t* ctx,
		IppRotateMode * rotateMode)
{
	switch (ctx->mTransformation)
	{
	case COPYBIT_TRANSFORM_ROT_90:
		*rotateMode = ippRotate90R;
		break;

	case COPYBIT_TRANSFORM_ROT_180:
		*rotateMode = ippRotate180;
		break;

	case COPYBIT_TRANSFORM_ROT_270:
		*rotateMode = ippRotate90L;
		break;

	default:
		*rotateMode = ippRotateDisable;
		break;
	}

	return 0;
}



int marvell_mmx_copybit(
		struct copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *rect,
		IppBlendMode blendMode,
		Ipp8u mPlaneAlpha)
{
	IppRect srcrect, dstrect;
	IppSurface ippsrc, ippdst;
	IppRotateMode rotateMode;
	struct copybit_rect_t dirty_rect;


	//LOGE("enter %s", __FUNCTION__);

	set_mmx_surface_parameter(src, &ippsrc, src_rect, &srcrect);
	set_mmx_surface_parameter(dst, &ippdst, dst_rect, &dstrect);
	set_mmx_rotate_parameter(ctx, &rotateMode);

	memcpy(&dirty_rect, rect, sizeof(struct copybit_rect_t));

#ifdef COPYBIT_WMMXLIB_DEBUG
	LOGE("1:wmmx srcrect(l%d,t%d,r%d,b%d), dstrect(l%d,t%d,r%d,b%d)\n",
			srcrect.left,
			srcrect.top,
			srcrect.right,
			srcrect.bottom,
			dstrect.left,
			dstrect.top,
			dstrect.right,
			dstrect.bottom);
	LOGE("2:wmmx srcf(pB%p,w%d,h%d,fmt%d,stride%d), dstf(pB%p,w%d,h%d,fmt%d,stride%d)\n",
			ippsrc.pBuf,
			ippsrc.width,
			ippsrc.height,
			ippsrc.clrFormat,
			ippsrc.stride,
			ippdst.pBuf,
			ippdst.width,
			ippdst.height,
			ippdst.clrFormat,
			ippdst.stride);
	LOGE("3: Dirty rect(l%d,t%d,r%d,b%d)\n",
			dirty_rect.l,
			dirty_rect.t,
			dirty_rect.r,
			dirty_rect.b);
#endif

	if (ippStsNoErr != ippComposeSurface(
		&ippsrc, &srcrect,
		&ippdst, &dstrect,
		(const IppRect*)(&dirty_rect), blendMode, mPlaneAlpha, rotateMode))
	{
		LOGE("%s: call ippComposeSurface error\n", __func__);
		return -EINVAL;
	}

	return NO_ERROR;

}

#ifdef __cplusplus
}
#endif
