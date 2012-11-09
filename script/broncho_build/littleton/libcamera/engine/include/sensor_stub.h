/*******************************************************************************
//(C) Copyright [2009] Marvell International Ltd.
//All Rights Reserved
*******************************************************************************/

#ifndef _SENSOR_STUB_H_
#define _SENSOR_STUB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <linux/ioctl.h>
#include <linux/videodev2.h>

#include "CamGen.h"
#include "CamRawCam.h"
#include "CamSubComp.h"
#include "log.h"


/*
 * Register Mapping
 *
 * every macro is the first 4 parameters for _SET_SENSOR_REG/_GET_SENSOR_REG.
 * 
 * for example, in (0x307c,1,3,0), "0x307c,1,3" means bit 1~3 of the 8-bit
 * register 0x307c, and "0" means the start LSB of 16-bit "val". 
 */




/*
 * Register Setting Tables
 */
typedef struct {
    const CAM_Int16u *pEntry; /* points to a serial of register settings */
    const int iEntryLength;   /* length of the entry */
} CAM_RegTableEntry;

#define _ARRAY_SIZE(array) ( sizeof(array)/sizeof((array)[0]) )


/*
 * Parameters
 */

extern CAM_SensorResolution _ResTable[];
extern const int _sizeResTable;


/*
 * Utility Functions
 */
//#define QCI

#ifdef QCI
struct reg_set_s {
    int  val1;
    int  val2;
};

#define WCAM_VIDIOSENSORR        _IOWR('v', 217, struct reg_set_s)
#define WCAM_VIDIOSENSORW        _IOW('v', 218, struct reg_set_s)
#else
struct v4l2_register {
    __u32 match_type; /* Match type */
    __u32 match_chip; /* Match this chip, meaning determined by match_type */
    __u64 reg;
    __u64 val;
};

#define VIDIOC_DBG_S_REGISTER    _IOW('V', 79, struct v4l2_register)
#define VIDIOC_DBG_G_REGISTER   _IOWR('V', 80, struct v4l2_register)
#endif
 
typedef struct {
     CAM_Int16u iStillShutter;
     CAM_Int16u iStillExtra;
     CAM_Int16u iStillGain16;
     CAM_Int16u iStillDummyLine;
     CAM_Int16u iStillDummyPix;
} CAM_AECState;

int __GET_SENSOR_REG(int iDeviceID, CAM_Int16u reg, int lbit, int hbit, int val_lbit, CAM_Int16u *val);
int __SET_SENSOR_REG(int iDeviceID, CAM_Int16u reg, int lbit, int hbit, int val_lbit, CAM_Int16u val);
#define _GET_SENSOR_REG(iDeviceID, reg, val)\
    (__GET_SENSOR_REG(iDeviceID, reg, val) ? (TRACE("reg = %s\n", #reg), 1) : 0)
#define _SET_SENSOR_REG(iDeviceID, reg, val)\
    (__SET_SENSOR_REG(iDeviceID, reg, val) ? (TRACE("reg = %s\n", #reg), 1) : 0) 
extern CAM_Error SetRegArray(int iDeviceID, const CAM_Int16u *p, int size);
extern CAM_Error _calc_still_exposure(CAM_Int16u iDeviceID, CAM_AECState *pAECState);
extern CAM_Error _set_still_exposure(CAM_Int16u iDeviceID, const CAM_AECState *pAECState);

#ifdef __cplusplus
}
#endif

#endif  // _SENSOR_STUB_H_
