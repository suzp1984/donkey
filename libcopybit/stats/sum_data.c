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
#define LOG_TAG "COPYBIT_TEST"
#include <utils/Log.h>


#include "../copybit_inc.h"

#include "sum_def.h"



#define MYTRACE	LOGD

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C"{
#endif



static int dumpsec = -1;
static int dumpusec = -1;
static int startsec = -1;
static int startusec = -1;


static float pixelnum[20];
static float elapse[20];




void log_copybit_start()
{
	int i;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	startsec = tv.tv_sec;
	startusec = tv.tv_usec;

	// clear the buffer in the first call
	if (dumpsec == -1)
	{
		for (i=0 ; i < (int)USE_CASE_NUM ; i++)
		{
			pixelnum[i] = 0.0f;
			elapse[i] = 0.0f;
		}

		dumpsec = startsec;
		dumpusec = startusec;
	}
}

void log_copybit_end(
		struct copybit_context_t *ctx,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *src_rect,
		struct copybit_image_t const *dst,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *rect)
{
	int i;
	int bResize = 0, bRotate = 0, bPixelAlpha = 0, bGlobalAlpha = 0, bBlur = 0, bDither = 0, bCopy = 0;
	int iSrcW = src_rect->r - src_rect->l;
	int iSrcH = src_rect->b - src_rect->t;
	int iDstW = dst_rect->r - dst_rect->l;
	int iDstH = dst_rect->b - dst_rect->t;
	struct copybit_rect_t dirtyrect = *rect;
	int iDirtW, iDirtH;
	float fDirtPixelNum;
	struct timeval tv;
	time_t curtime;
	char buffer[1024];
	float copybit_elapse, dump_interval;

	gettimeofday(&tv, NULL);
	copybit_elapse   = tv.tv_sec - startsec + (tv.tv_usec - startusec) / 1000000.0f;
	dump_interval    = tv.tv_sec - dumpsec  + (tv.tv_usec - dumpusec)  / 1000000.0f;

	if (dirtyrect.l < dst_rect->l)
		dirtyrect.l = dst_rect->l;

	if (dirtyrect.r > dst_rect->r)
		dirtyrect.r = dst_rect->r;

	if (dirtyrect.t < dst_rect->t)
		dirtyrect.t = dst_rect->t;

	if (dirtyrect.b > dst_rect->b)
		dirtyrect.b = dst_rect->b;

	if (dirtyrect.l >= dirtyrect.r || dirtyrect.t >= dirtyrect.b)
		return;

	iDirtW = dirtyrect.r - dirtyrect.l;
	iDirtH = dirtyrect.b - dirtyrect.t;
	fDirtPixelNum = iDirtW * iDirtH / 1024.0f / 1024.0f;

	bRotate = ctx->mTransformation != 0;
	bResize = (bRotate && (iSrcW != iDstH || iSrcH != iDstW)) || (!bRotate && (iSrcW != iDstW || iSrcH != iDstH));
	bPixelAlpha = (src->format == COPYBIT_FORMAT_RGBA_8888 ||
		src->format == COPYBIT_FORMAT_BGRA_8888 ||
		src->format == COPYBIT_FORMAT_RGBA_5551 ||
		src->format == COPYBIT_FORMAT_RGBA_4444);
	bGlobalAlpha = ctx->mPlaneAlpha != 255;

	bDither = ctx->mDitherEnabled;
	bBlur = ctx->mBlurEnabled;

	bCopy = !bResize && !bRotate && !bPixelAlpha && !bGlobalAlpha && !bBlur && !bDither;

	// do throughput statistic
	for (i=0 ; i < (int)USE_CASE_NUM ; i++)
	{
		if (use_case[i].eSrcFormat == src->format &&
			use_case[i].eDstFormat == dst->format &&
			!(use_case[i].bResize ^ bResize) &&
			!(use_case[i].bRotate ^ bRotate) &&
			!(use_case[i].bGlobalAlpha ^ bGlobalAlpha) &&
			!(use_case[i].bPixelAlpha ^ bPixelAlpha))
		{
			pixelnum[i] += fDirtPixelNum;
			elapse[i] += copybit_elapse;
			break;
		}
	}
	if (i == USE_CASE_NUM)
	{
		MYTRACE("Failed to do statistics!\n");
	}

	// output throughput statistic data at "DUMP_INTERVAL"
	if (dump_interval > DUMP_INTERVAL)
	{
		MYTRACE("Throughput (MP/s)\tUse Case\n");
		for (i=0 ; i < (int)USE_CASE_NUM ; i++)
		{
			if (pixelnum[i] > 0.0)
			{
				MYTRACE("%.3f\t\t%s %s %s%s%s%s", pixelnum[i] / elapse[i],
					GetFormatString(use_case[i].eSrcFormat),
					GetFormatString(use_case[i].eDstFormat),
					use_case[i].bResize ? "Rsz " : "",
					use_case[i].bRotate ? "Rot " : "",
					use_case[i].bGlobalAlpha ? "GA " : "",
					use_case[i].bPixelAlpha ? "PA " : "");
			}
		}

		dumpsec += DUMP_INTERVAL;
	}
}


#ifdef __cplusplus
}
#endif

