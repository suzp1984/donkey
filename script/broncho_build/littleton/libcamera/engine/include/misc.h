/******************************************************************************
// (C) Copyright 2008 Marvell International Ltd.
// All Rights Reserved
******************************************************************************/
#ifndef _MISC_H_
#define _MISC_H_

#include <stdlib.h>
#include <stdio.h>

#ifndef Symbian
#include <memory.h>
#endif

#include "codecDef.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int miscgMemMalloc(void **ppDstBuf, int size, unsigned char align);
extern int miscgMemCalloc(void **ppDstBuf, int size, unsigned char align);
extern int miscgMemFree(void **ppSrcBuf);
extern int miscgStreamFlush(void **ppStream, void *pStreamHandle, int nAvailBytes, int extMode);

extern int miscInitGeneralCallbackTable(MiscGeneralCallbackTable **ppDstCBTable);
extern int miscFreeGeneralCallbackTable(MiscGeneralCallbackTable **ppSrcCBTable);
extern int miscSeekFileCallBack(void *handler, int offset, int start);
extern int miscReadFileCallBack(void *pBuf, int unit, int number, void *handler);
extern int miscWriteFileCallBack(void *pBuf, int unit, int number, void *handler);

#ifdef __cplusplus
}
#endif

#endif

