/*******************************************************************************
//      (C) Copyright 2009 Marvell International Ltd.
//          All Rights Reserved
********************************************************************************/

#ifndef _COMPSURF_H_
#define _COMPSURF_H_

#include "ippdefs.h"

#ifdef __cplusplus
extern "C" {
#endif


//*****************************************************************************
// MACROS
//*****************************************************************************
#define COMPSURF_TRUE    1
#define COMPSURF_FALSE   0
#define COMPSURF_NULL    0
typedef enum {
	ippColorFmtRGB565,
	//  ippColorFmtARGB1555,
	ippColorFmtRGB888,
	ippColorFmtARGB8888,
	ippColorFmtABGR8888,

	ippColorFmtCount
} IppColorFmt; // Color formats

typedef enum {
	ippRotateDisable,    // No transform
	ippRotate90L,        // Rotated counterclockwise by 90 degrees.
	ippRotate180,        // Rotated clockwise by 180 degrees.
	ippRotate90R,        // Rotated clockwise by 90 degrees.
	ippMirror,           // Reflected vertical center.
	ippMirrorRotate90L,  // Reflected vertical center and then rotated counterclockwise by 90 degrees
	ippMirrorRotate180,  // Reflected vertical center and then rotated clockwise by 180 degrees
	ippMirrorRotate90R,  // Reflected vertical center and then rotated clockwise by 90 degrees.

	ippRotateModeCount,
} IppRotateMode;

typedef enum {
	ippBlendCopy,
	ippBlendSrcOvr,
	ippBlendAlphaMul,
} IppBlendMode;

typedef struct  _IppSurface{
	Ipp8u*      pBuf;           // Pointer to the buffer
	Ipp16u      width;          // Width in Pixels
	Ipp16u      height; 	    // Height in Pixels
	Ipp32s      stride;         // Pitch/Stride in bytes
	IppColorFmt clrFormat;      // Color Format
} IppSurface;

typedef struct _IppRect {
	Ipp32s     left,top,right,bottom;
} IppRect;



//*****************************************************************************
// GLOBAL VARIABLES
//*****************************************************************************


//*****************************************************************************
// FUNCTIONS
//*****************************************************************************


IppStatus ippComposeSurface(const IppSurface* pSrcSurface,
		const IppRect* pSrcRect,
		IppSurface* pDstSurface,
		const IppRect* pDstRect,
		const IppRect* pDirtyRect,
		IppBlendMode blendMode,
		Ipp8u iAlphaQ8,
		IppRotateMode rotateMode);

#ifdef __cplusplus
}
#endif

#endif // _COMPSURF_H_
