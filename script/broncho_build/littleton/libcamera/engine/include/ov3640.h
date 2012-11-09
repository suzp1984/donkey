/*******************************************************************************
//(C) Copyright [2009] Marvell International Ltd.
//All Rights Reserved
*******************************************************************************/

#ifndef _OV3640_H_
#define _OV3640_H_

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
 * OV3640 Register Mapping
 *
 * every macro is the first 4 parameters for _SET_SENSOR_REG/_GET_SENSOR_REG.
 * 
 * for example, in (0x307c,1,3,0), "0x307c,1,3" means bit 1~3 of the 8-bit
 * register 0x307c, and "0" means the start LSB of 16-bit "val". 
 */
#define OV3640_REG_NIGHTENABLE      0x3014,3,3,0
#define OV3640_REG_DUMMYFRMCTRL     0x3015,4,6,0

#define OV3640_REG_MIRROR           0x307c,1,1,0
#define OV3640_REG_FLIP             0x307c,0,0,0
#define OV3640_REG_YAVG             0x301b,0,7,0
#define OV3640_REG_ANAGAIN          0x3001,0,7,0
#define OV3640_REG_DIGGAINEN        0x307c,5,5,0
#define OV3640_REG_DIGGAIN          0x307e,6,7,0
#define OV3640_REG_AECEXPTIMEH      0x3002,0,7,8
#define OV3640_REG_AECEXPTIMEL      0x3003,0,7,0
#define OV3640_REG_MANUALWB         0x332b,3,3,0
#define OV3640_REG_RGAIN            0x33a7,0,7,0
#define OV3640_REG_GGAIN            0x33a8,0,7,0
#define OV3640_REG_BGAIN            0x33a9,0,7,0

// 50/60Hz banding filter
#define OV3640_REG_AUTOBANDENABLE   0x3014,6,6,0
#define OV3640_REG_MANUALBANDMODE   0x3014,7,7,0
#define OV3640_REG_AUTOBANDMODE     0x3013,4,4,0
#define OV3640_REG_BANDFILTERENABLE 0x3013,5,5,0
#define OV3640_REG_50HZBANDMSB      0x3070,0,7,8
#define OV3640_REG_50HZBANDLSB      0x3071,0,7,0
#define OV3640_REG_60HZBANDMSB      0x3072,0,7,8
#define OV3640_REG_60HZBANDLSB      0x3073,0,7,0
#define OV3640_REG_50HZMAXBANDSTEP  0x301c,0,5,0
#define OV3640_REG_60HZMAXBANDSTEP  0x301d,0,5,0

// FPS control
#define OV3640_REG_PLL1             0x300e,0,5,0
#define OV3640_REG_PLL2             0x300f,6,7,0
#define OV3640_REG_PLL3             0x300f,0,1,0
#define OV3640_REG_PLL4             0x3010,4,4,0
#define OV3640_REG_PLL5             0x3011,0,5,0

// zoom/crop registers
#define OV3640_REG_ZOOMENABLE       0x3302,5,5,0
#define OV3640_REG_ZOOMVSIZEINHIGH  0x335f,4,6,8
#define OV3640_REG_ZOOMHSIZEINHIGH  0x335f,0,3,8
#define OV3640_REG_ZOOMHSIZEINLOW   0x3360,0,7,0
#define OV3640_REG_ZOOMVSIZEINLOW   0x3361,0,7,0
#define OV3640_REG_ZOOMVSIZEOUTHIGH 0x3362,4,6,8
#define OV3640_REG_ZOOMHSIZEOUTHIGH 0x3362,0,3,8
#define OV3640_REG_ZOOMHSIZEOUTLOW  0x3363,0,7,0
#define OV3640_REG_ZOOMVSIZEOUTLOW  0x3364,0,7,0
#define OV3640_REG_CROPHSTARTHIGH   0x3020,0,7,8
#define OV3640_REG_CROPHSTARTLOW    0x3021,0,7,0
#define OV3640_REG_CROPVSTARTHIGH   0x3022,0,7,8
#define OV3640_REG_CROPVSTARTLOW    0x3023,0,7,0
#define OV3640_REG_CROPWIDTHHIGH    0x3024,0,7,8
#define OV3640_REG_CROPWIDTHLOW     0x3025,0,7,0
#define OV3640_REG_CROPHEIGHTHIGH   0x3026,0,7,8
#define OV3640_REG_CROPHEIGHTLOW    0x3027,0,7,0

// AEC/AGC registers
#define OV3640_REG_AECENABLE        0x3013,0,0,0
#define OV3640_REG_AGCENABLE        0x3013,2,2,0
#define OV3640_REG_AECHISHIGH       0x3018,0,7,0
#define OV3640_REG_AECHISLOW        0x3019,0,7,0
#define OV3640_REG_VPT              0x301a,0,7,0
#define OV3640_REG_AEGCMETHODSELECT 0x3047,7,7,0
#define OV3640_REG_AGCGAINH         0x3000,0,7,8
#define OV3640_REG_AGCGAINL         0x3001,0,7,0
#define OV3640_REG_EXPLINEHIGH      0x3002,0,7,8
#define OV3640_REG_EXPLINELOW       0x3003,0,7,0
#define OV3640_REG_DUMMYLINEHIGH    0x302a,0,7,8
#define OV3640_REG_DUMMYLINELOW     0x302b,0,7,0
#define OV3640_REG_EXTRALINEHIGH    0x302d,0,7,8
#define OV3640_REG_EXTRALINELOW     0x302e,0,7,0
#define OV3640_REG_DUMMYPIXHIGH     0x3028,0,7,8
#define OV3640_REG_DUMMYPIXLOW      0x3029,0,7,0

// special digital effect registers
#define OV3640_REG_SDEENABLE        0x3302,7,7,0
#define OV3640_REG_HUECOSSIGN       0x3354,4,5,0
#define OV3640_REG_HUESINSIGN       0x3354,0,1,0
#define OV3640_REG_YOFFSETSIGN      0x3354,2,2,0
#define OV3640_REG_YBRIGHTSIGN      0x3354,3,3,0
#define OV3640_REG_FIXEDYENABLE     0x3355,7,7,0
#define OV3640_REG_NEGTIVEENABLE    0x3355,6,6,0
#define OV3640_REG_GRAYENABLE       0x3355,5,5,0
#define OV3640_REG_FIXEDUENABLE     0x3355,4,4,0
#define OV3640_REG_FIXEDVENABLE     0x3355,3,3,0
#define OV3640_REG_CONTRASTENABLE   0x3355,2,2,0
#define OV3640_REG_SATURATIONENABLE 0x3355,1,1,0
#define OV3640_REG_HUEENABLE        0x3355,0,0,0
#define OV3640_REG_HUECOS           0x3356,0,7,0
#define OV3640_REG_HUESIN           0x3357,0,7,0
#define OV3640_REG_SATU             0x3358,0,7,0
#define OV3640_REG_SATV             0x3359,0,7,0
#define OV3640_REG_UREG             0x335a,0,7,0
#define OV3640_REG_VREG             0x335b,0,7,0
#define OV3640_REG_YOFFSET          0x335c,0,7,0
#define OV3640_REG_YGAIN            0x335d,0,7,0
#define OV3640_REG_YBRIGHT          0x335e,0,7,0

// average AEC measure window
#define OV3640_REG_AHS_HIGH         0x3038,0,3,8
#define OV3640_REG_AHS_LOW          0x3039,0,7,0
#define OV3640_REG_AVS_HIGH         0x303a,0,2,8
#define OV3640_REG_AVS_LOW          0x303b,0,7,0
#define OV3640_REG_AHW_HIGH         0x303c,0,3,8
#define OV3640_REG_AHW_LOW          0x303d,0,7,0
#define OV3640_REG_AVH_HIGH         0x303e,0,3,8
#define OV3640_REG_AVH_LOW          0x303f,0,7,0
#define OV3640_REG_AVSECTION_1_2    0x3030,0,7,0
#define OV3640_REG_AVSECTION_3_4    0x3031,0,7,0
#define OV3640_REG_AVSECTION_5_6    0x3032,0,7,0
#define OV3640_REG_AVSECTION_7_8    0x3033,0,7,0
#define OV3640_REG_AVSECTION_9_10   0x3034,0,7,0
#define OV3640_REG_AVSECTION_11_12  0x3035,0,7,0
#define OV3640_REG_AVSECTION_13_14  0x3036,0,7,0
#define OV3640_REG_AVSECTION_15_16  0x3037,0,7,0

// Firmware related registers
#define OV3640_REG_FWCMD            0x3f00,0,7,0
#define OV3640_REG_FWSTATE          0x3f01,0,7,0
// Firmware states
#define OV3640_FWSTATE_INF               0x00
#define OV3640_FWSTATE_SINGLE            0x65
#define OV3640_FWSTATE_SUCCESS_S         0x46
#define OV3640_FWSTATE_FAIL_S            0xc6
#define OV3640_FWSTATE_CAPTURE_S         0x47


/*
 * OV3640 Register Setting Tables
 */
typedef struct {
    const CAM_Int16u *pEntry; /* points to a serial of register settings */
    const int iEntryLength;   /* length of the entry */
} CAM_RegTableEntry;

extern CAM_RegTableEntry _OV3640AWBTable[];
extern CAM_RegTableEntry _OV3640ColorEffectTable[];
extern CAM_RegTableEntry _OV3640ISOTable[];
extern CAM_RegTableEntry _OV3640SatTable[];
extern CAM_RegTableEntry _OV3640ContrastTable[];
extern CAM_RegTableEntry _OV3640EVTable[];
extern CAM_RegTableEntry _OV3640ExposureMeasureTable[];
extern CAM_RegTableEntry _OV3640BandFilterTable[];

#define _ARRAY_SIZE(array) ( sizeof(array)/sizeof((array)[0]) )

extern const int _sizeOV3640AWBTable;
extern const int _sizeOV3640ColorEffectTable;
extern const int _sizeOV3640ISOTable;
extern const int _sizeOV3640SatTable;
extern const int _sizeOV3640ContrastTable;
extern const int _sizeOV3640EVTable;
extern const int _sizeOV3640ExposureMeasureTable;
extern const int _sizeOV3640BandFilterTable;


/*
 * OV3640 Parameters
 */
#define OV3640_WIDTH_QXGA               2048
#define OV3640_HEIGHT_QXGA              1536
#define OV3640_WIDTH_XGA                1024
#define OV3640_HEIGHT_XGA               768
#define OV3640_SHUTTER_MAX_XGA          779
#define OV3640_SHUTTER_MAX_QXGA         1568
#define OV3640_LINEWIDTH_DEFAULT_XGA    1188
#define OV3640_LINEWIDTH_DEFAULT_QXGA   2376
#define PREVIEW_PIXEL_CLK               56
#define CAPTURE_PIXEL_CLK               112
#define QXGA_SHUTTER_MAX                1563
#define CAPTURE_GAIN16_MAX              128 // 8x
#define DEFAULT_XGA_LINE_WIDTH          1188
#define DEFAULT_QXGA_LINE_WIDTH         2376
#define DEFAULT_REG_0X3028              0x09
#define DEFAULT_REG_0X3029              0x47
#define DEFAULT_XGA_REG_0x302a          0x03
#define DEFAULT_XGA_REG_0x302b          0x10
#define DEFAULT_QXGA_REG_0x302a         0x06
#define DEFAULT_QXGA_REG_0x302b         0x20

extern CAM_SensorResolution _OV3640ResTable[];
extern const int _sizeOV3640ResTable;


/*
 * OV3640 Utility Functions
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
} OV3640_AECState;

int __GET_SENSOR_REG(int iDeviceID, CAM_Int16u reg, int lbit, int hbit, int val_lbit, CAM_Int16u *val);
int __SET_SENSOR_REG(int iDeviceID, CAM_Int16u reg, int lbit, int hbit, int val_lbit, CAM_Int16u val);
#define _GET_SENSOR_REG(iDeviceID, reg, val)\
    (__GET_SENSOR_REG(iDeviceID, reg, val) ? (TRACE("reg = %s\n", #reg), 1) : 0)
#define _SET_SENSOR_REG(iDeviceID, reg, val)\
    (__SET_SENSOR_REG(iDeviceID, reg, val) ? (TRACE("reg = %s\n", #reg), 1) : 0) 
extern CAM_Error SetRegArray(int iDeviceID, const CAM_Int16u *p, int size);
extern CAM_Error _calc_still_exposure(CAM_Int16u iDeviceID, OV3640_AECState *pAECState);
extern CAM_Error _set_still_exposure(CAM_Int16u iDeviceID, const OV3640_AECState *pAECState);

#ifdef __cplusplus
}
#endif

#endif  // _OV3640_H_
