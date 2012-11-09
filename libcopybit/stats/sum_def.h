/*
 * sum_def.h
 *
 *  Created on: Jan 15, 2010
 *      Author: archermind
 */

#ifndef SUM_DEF_H_
#define SUM_DEF_H_


#ifdef __cplusplus
extern "C"{
#endif

#include <pthread.h>



typedef struct tagTimeSum
{
	pthread_mutex_t	mMutex;
	long nMicroSeconds;
	long nCurCount;
	long nMaxCount;

	int mRotationDegree;
	int mPlaneAlpha;
	int mDitherEnabled;
	int mBlurEnabled;
	int mTransformation;
	int mTransformationLast;

	int mUseCaseIndex;
	long mIntervalSeconds;
	long mOutputTime;
}TimeSum;

typedef struct _use_case {
	int eSrcFormat;
	int eDstFormat;
	int bResize;
	int bRotate;
	int bGlobalAlpha;
	int bPixelAlpha;
} UseCase;



#define USE_CASE_NUM	6
#define DUMP_INTERVAL	2

extern const UseCase use_case [USE_CASE_NUM];

const char *GetFormatString(int format);

#ifdef __cplusplus
}
#endif

#endif /* SUM_DEF_H_ */
