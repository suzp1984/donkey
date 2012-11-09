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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include<sys/time.h>
#include<signal.h>


#include "hardware/copybit.h"

#include "sum_def.h"
#include "sum_time.h"

#undef LOG_TAG
#define LOG_TAG "COPYBIT_TEST"
#include <utils/Log.h>


#ifdef __cplusplus
extern "C"{
#endif




#ifdef STATS_DEBUG
	#define TRACE			LOGE
#else
	#define TRACE(szFmt, args...)
#endif


#define SUM_FILE	"/data/copybit.txt"
#define SPEED_FILE	"/data/speed.txt"
#define MAX_COUNT	1000



const UseCase use_case [USE_CASE_NUM] = {
	{COPYBIT_FORMAT_RGB_565,   COPYBIT_FORMAT_RGB_565, 0, 0, 0, 0},
	{COPYBIT_FORMAT_RGB_565,   COPYBIT_FORMAT_RGB_565, 0, 1, 0, 0},
	{COPYBIT_FORMAT_RGB_565,   COPYBIT_FORMAT_RGB_565, 1, 0, 1, 0},
	{COPYBIT_FORMAT_RGB_565,   COPYBIT_FORMAT_RGB_565, 1, 1, 1, 0},
	{COPYBIT_FORMAT_RGBA_8888, COPYBIT_FORMAT_RGB_565, 1, 0, 1, 1},
	{COPYBIT_FORMAT_RGBA_8888, COPYBIT_FORMAT_RGB_565, 1, 1, 1, 1},
};

static TimeSum m_gTimeSum;
static float pixelnum[USE_CASE_NUM] = {0};
static float elapse[USE_CASE_NUM] = {0};


static int sum_one_rect(
		struct copybit_image_t *dst,
		struct copybit_image_t *src,
		struct copybit_rect_t *dst_rect,
		struct copybit_rect_t *src_rect,
		struct copybit_rect_t *rect);

static void save_data(TimeVal * pEndTime);
static void save_time();



void sum_init(void)
{
	char  szLine[256] = {0};
	char * pLine = NULL;
	FILE * fp = NULL;

	memset(&m_gTimeSum, 0, sizeof(TimeSum));

	if (NULL == (fp = fopen(SUM_FILE, "r")))
	{
		goto Speed;
	}

	if (NULL != fgets(szLine, 256, fp))
	{
		char * pLine = strchr(szLine, '\n');
		if (NULL != pLine)
		{
			*pLine = 0;
			m_gTimeSum.nMaxCount = atol(szLine);
		}
	}

	fclose(fp);

Speed:
	if (NULL == (fp = fopen(SPEED_FILE, "r")))
	{
		goto End;
	}

	if (NULL != fgets(szLine, 256, fp))
	{
		char * pLine = strchr(szLine, '\n');
		if (NULL != pLine)
		{
			*pLine = 0;
			m_gTimeSum.mIntervalSeconds = atol(szLine);
		}
	}

End:
	if (NULL != fp)
		fclose(fp);

	if (m_gTimeSum.nMaxCount <= 0)
		m_gTimeSum.nMaxCount = MAX_COUNT;

	if (m_gTimeSum.mIntervalSeconds <= 0)
		m_gTimeSum.mIntervalSeconds = 5;

	LOGE("set nMaxCount to %ld from %s", m_gTimeSum.nMaxCount, SUM_FILE);
	LOGE("set Interval to %ld from %s", m_gTimeSum.mIntervalSeconds, SPEED_FILE);
	pthread_mutex_init(&m_gTimeSum.mMutex, NULL);
}


void sum_destroy(void)
{
	pthread_mutex_destroy(&m_gTimeSum.mMutex);
}


void sum_lock(void)
{
	pthread_mutex_lock(&m_gTimeSum.mMutex);
}

void sum_unlock(void)
{
	pthread_mutex_unlock(&m_gTimeSum.mMutex);
}



void sum_set_parameter(int name, int value)
{
	switch(name)
	{
	case COPYBIT_ROTATION_DEG:
		LOGW("COPYBIT_ROTATION_DEG is not supported.\n");
		break;
	case COPYBIT_PLANE_ALPHA:
		value = value > 0xFF ? 0xFF : value;
		value = value < 0 ? 0 : value;
		m_gTimeSum.mPlaneAlpha = value;
		break;
	case COPYBIT_DITHER:
		// FIXME: GC300 current doesn't support DITHER
		if(value ==COPYBIT_DISABLE || value == COPYBIT_ENABLE) {
			m_gTimeSum.mDitherEnabled = value;
		}
		else {
			LOGW("Invalid dither enabling flag: %d\n", value);
		}
		break;
	case COPYBIT_BLUR:
		if(value ==COPYBIT_DISABLE || value == COPYBIT_ENABLE) {
			m_gTimeSum.mBlurEnabled = value;
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
				m_gTimeSum.mTransformation = value;
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
}



int sum_all_rect(
		struct copybit_image_t *dst,
		struct copybit_image_t *src,
		struct copybit_rect_t *dst_rect,
		struct copybit_rect_t *src_rect,
		struct copybit_region_t *region)
{
	struct copybit_rect_t rect;
	struct copybit_region_t * pRegion = region;
	int nLastUseCase = -2;

	while (pRegion->next(pRegion, &rect))
	{
		int nCurUseCase = sum_one_rect(dst, src, dst_rect, src_rect, &rect);

		if (nLastUseCase == -2)
			nLastUseCase = nCurUseCase;

		if (nLastUseCase != nCurUseCase)
		{
			LOGE("Find a different use case index, nLastUseCase: %d, nCurUseCase: %d!", nLastUseCase, nCurUseCase);
		}
	}

	return nLastUseCase;
}


#ifdef STATS_DEBUG
void start_sum(char * szFile, int nLine, TimeVal * pStartTime)
#else
void start_sum(TimeVal * pStartTime)
#endif
{
	memset(pStartTime, 0, sizeof(TimeVal));
	if (gettimeofday(pStartTime, NULL) < 0)
	{
		LOGE("Failed to get start time: %s!", strerror(errno));
		return;
	}

	TRACE("[%s %d] pStartTime: %ld", szFile, nLine, pStartTime->tv_sec);
}


#ifdef STATS_DEBUG
void end_sum(char * szFile, int nLine, TimeVal * pStartTime, TimeVal * pEndTime, int nUseCase)
#else
void end_sum(TimeVal * pStartTime, TimeVal * pEndTime, int nUseCase)
#endif
{
	long usetime = 0;

	memset(pEndTime, 0, sizeof(TimeVal));
	if (gettimeofday(pEndTime, NULL) < 0)
	{
		LOGE("Failed to get end time %s! So we return!", strerror(errno));
		return;
	}

	TRACE("[%s %d] pEndTime: %ld, pStartTime: %ld, nUseCase: %d", \
			szFile, nLine, pEndTime->tv_sec, pStartTime->tv_sec, nUseCase);
	pthread_mutex_lock(&m_gTimeSum.mMutex);
	m_gTimeSum.nCurCount++;

	usetime = (pEndTime->tv_sec - pStartTime->tv_sec) * 1000000 \
					+ (pEndTime->tv_usec - pStartTime->tv_usec);

	if (usetime < 0)
	{
		LOGE("pEndTime  (tv_sec: %ld, tv_usec: %ld)", pEndTime->tv_sec, pEndTime->tv_usec);
		LOGE("pStartTime(tv_sec: %ld, tv_usec: %ld)", pStartTime->tv_sec, pStartTime->tv_usec);
		return;
	}

	m_gTimeSum.nMicroSeconds += usetime;

	if (-1 < m_gTimeSum.mUseCaseIndex)
		elapse[nUseCase] += usetime * 1.0f / 1000000;

	if (m_gTimeSum.nCurCount == m_gTimeSum.nMaxCount)
	{
		save_time();
	}

	save_data(pEndTime);
	pthread_mutex_unlock(&m_gTimeSum.mMutex);
}



static void save_time()
{
	float nAverageTime = m_gTimeSum.nMicroSeconds * 1.0 / m_gTimeSum.nMaxCount;
	FILE * fp = fopen(SUM_FILE, "a+");

	char szTime[24] = {0};
	time_t tm = time(NULL);

	if (NULL == fp)
	{
		return;
	}

	strftime(szTime, sizeof(szTime), "%H:%M:%S", localtime(&tm));
	fprintf(fp, "[%s] SumCount: %4ld, SumTime: %6ld microsecond, AverageTime: %4.3f microsecond\n",\
			szTime, m_gTimeSum.nMaxCount, m_gTimeSum.nMicroSeconds, nAverageTime);

	fclose(fp);

	m_gTimeSum.nMicroSeconds = 0;
	m_gTimeSum.nCurCount = 0;
}



// output throughput statistic data at "DUMP_INTERVAL"
static void save_data(TimeVal * pEndTime)
{
	int i;
	FILE * fp = NULL;
	long nCurTime = pEndTime->tv_sec;

	TRACE("nCurTime: %ld, Last output time: %ld", nCurTime, m_gTimeSum.mOutputTime);
	TRACE("elapse: %ld", nCurTime - m_gTimeSum.mOutputTime);

	if (nCurTime - m_gTimeSum.mOutputTime > m_gTimeSum.mIntervalSeconds)
	{
		char szTime[24] = {0};
		time_t tm = time(NULL);

		if (NULL == (fp = fopen(SPEED_FILE, "a+")))
		{
			return;
		}

		strftime(szTime, sizeof(szTime), "%H:%M:%S", localtime(&tm));
		fprintf(fp, "[%s] Throughput (MP/s)\tUse Case\n", szTime);
		for (i=0 ; i < (int)USE_CASE_NUM ; i++)
		{
			TRACE("pixelnum[%d]: %f, elapse[%d]: %f", i, pixelnum[i], i, elapse[i]);
			if (pixelnum[i] > 0.0 && elapse[i] > 0.0)
			{
				fprintf(fp, "        speed: %.3f\t %s %s %s %s %s %s\n",
						pixelnum[i] / elapse[i],
						GetFormatString(use_case[i].eSrcFormat),
						GetFormatString(use_case[i].eDstFormat),
						use_case[i].bResize ? "resize" : "",
						use_case[i].bRotate ? "rotate" : "",
						use_case[i].bGlobalAlpha ? "gloal alpha" : "",
						use_case[i].bPixelAlpha ? "pixel alpha" : "");
			}
		}

		fclose(fp);
		m_gTimeSum.mOutputTime = nCurTime;

		TRACE("Reset pixelnum and elapse!");
		for (i = 0; i < (int) USE_CASE_NUM; i++)
		{
			pixelnum[i] = 0.0;
			elapse[i] = 0.0;
		}
	}
}




static int sum_one_rect(
		struct copybit_image_t *dst,
		struct copybit_image_t *src,
		struct copybit_rect_t *dst_rect,
		struct copybit_rect_t *src_rect,
		struct copybit_rect_t *rect)
{
	int i;
	struct copybit_rect_t dirtyrect = *rect;
	int bResize = 0, bRotate = 0;
	int bPixelAlpha = 0, bGlobalAlpha = 0;

	int iSrcW = src_rect->r - src_rect->l;
	int iSrcH = src_rect->b - src_rect->t;
	int iDstW = dst_rect->r - dst_rect->l;
	int iDstH = dst_rect->b - dst_rect->t;

	int iDirtW, iDirtH;
	float fDirtPixelNum;

	if (dirtyrect.l < dst_rect->l)
		dirtyrect.l = dst_rect->l;

	if (dirtyrect.r > dst_rect->r)
		dirtyrect.r = dst_rect->r;

	if (dirtyrect.t < dst_rect->t)
		dirtyrect.t = dst_rect->t;

	if (dirtyrect.b > dst_rect->b)
		dirtyrect.b = dst_rect->b;

	if (dirtyrect.l >= dirtyrect.r || dirtyrect.t >= dirtyrect.b)
	{
		LOGE("Invalid rect");
		return -1;
	}


	iDirtW = dirtyrect.r - dirtyrect.l;
	iDirtH = dirtyrect.b - dirtyrect.t;

	fDirtPixelNum = iDirtW * iDirtH / 1024.0f / 1024.0f;

	bRotate = m_gTimeSum.mTransformation != 0;
	bResize = (bRotate && (iSrcW != iDstH || iSrcH != iDstW))
			|| (!bRotate && (iSrcW != iDstW || iSrcH != iDstH));


	bPixelAlpha = (src->format == COPYBIT_FORMAT_RGBA_8888
				|| src->format == COPYBIT_FORMAT_BGRA_8888
				|| src->format == COPYBIT_FORMAT_RGBA_5551
				|| src->format == COPYBIT_FORMAT_RGBA_4444);


	bGlobalAlpha = m_gTimeSum.mPlaneAlpha != 255;


	// do throughput statistic
	for (i=0 ; i < USE_CASE_NUM; i++)
	{

		if (use_case[i].eSrcFormat == src->format &&
			use_case[i].eDstFormat == dst->format &&
			!(use_case[i].bResize ^ bResize) &&
			!(use_case[i].bRotate ^ bRotate) &&
			!(use_case[i].bGlobalAlpha ^ bGlobalAlpha) &&
			!(use_case[i].bPixelAlpha ^ bPixelAlpha))
		{
			pixelnum[i] += fDirtPixelNum;
			TRACE("case: %d", i);
			return i;
		}

	}


	TRACE("case: %d", -1);
	return -1;
}


const char *GetFormatString(int format)
{
	const char *string;

	switch (format)
	{
		case COPYBIT_FORMAT_RGBA_8888:
			string = "RGBA8888";
			break;
		case COPYBIT_FORMAT_BGRA_8888:
			string = "BGRA8888";
			break;
		case COPYBIT_FORMAT_RGB_565:
			string = "RGB565";
			break;
		case COPYBIT_FORMAT_RGBA_5551:
			string = "RGBA5551";
			break;
		case COPYBIT_FORMAT_RGBA_4444:
			string = "RGBA4444";
			break;
		case COPYBIT_FORMAT_YCbCr_422_SP:
			string = "YCbCr422SP";
			break;
		case COPYBIT_FORMAT_YCbCr_420_SP:
			string = "YCbCr420P";
			break;
		default:
			string = "<Unknown>";
			break;
	}

	return string;
}


#ifdef __cplusplus
}
#endif


