/*******************************************************************************
//(C) Copyright [2009] Marvell International Ltd.
//All Rights Reserved
*******************************************************************************/

#ifndef _LOG_H_
#define _LOG_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#ifdef ANDROID_CAMERAENGINE
	#include <utils/Log.h>
	#define LOG_TAG "CameraEngine"
	#define ERROR	LOGE(_ERROR_DOMAIN_"(%s, %d): ", __FILE__, __LINE__);LOGE
#ifdef CAM_LOG_VERBOSE
   	#define TRACE	LOGD
	#define VALIDATE(error)\
		if (error != CAM_ERROR_NONE)\
		{\
			TRACE("error code #%d at Function %s, Line %d\n", error, __FUNCTION__, __LINE__);\
		}
	#define ASSERT(v)\
		if (!(v))\
		{\
			TRACE("assertion failed: '%s' at Function %s, Line %d\n", #v, __FUNCTION__, __LINE__);\
		}
	#define MAKESURE(v)\
		if (!(v))\
		{\
			TRACE("assertion failed: '%s' at Function %s, Line %d\n", #v, __FUNCTION__, __LINE__);\
		}
	#define ECHO TRACE("%s, %d\n", __FILE__, __LINE__);
#else
 	#define TRACE
	#define VALIDATE(error)
	#define ASSERT(v)
	#define MAKESURE(v)			(v)
	#define ECHO
#endif //CAM_LOG_VERBOSE

#else
    #define ERROR	printf(_ERROR_DOMAIN_"(%s, %d): ", __FILE__, __LINE__);printf
#ifdef CAM_LOG_VERBOSE
	#define TRACE	printf
	#define VALIDATE(error)\
		if (error != CAM_ERROR_NONE)\
		{\
			TRACE("error code #%d at File %s, Line %d\n", error, __FILE__, __LINE__);\
		}
	#define ASSERT(v)\
		if (!(v))\
		{\
			TRACE("assertion failed: '%s' at File %s, Line %d\n", #v, __FILE__, __LINE__);\
		}
	#define MAKESURE(v)\
		if (!(v))\
		{\
			TRACE("assertion failed: '%s' at File %s, Line %d\n", #v, __FILE__, __LINE__);\
		}
	#define ECHO TRACE("%s, %d\n", __FILE__, __LINE__);
#else
 	#define TRACE
	#define VALIDATE(error)
	#define ASSERT(v)
	#define MAKESURE(v)			(v)
	#define ECHO
#endif //CAM_LOG_VERBOSE

#endif //ANDROID_CAMERAENGINE


#ifdef __cplusplus
}
#endif

#endif  // _LOG_H_
