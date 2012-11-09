/******************************************************************************
//(C) Copyright [2009] Marvell International Ltd.
//All Rights Reserved
******************************************************************************/


#ifndef _CAM_GEN_H_
#define _CAM_GEN_H_

#ifdef __cplusplus
extern "C" {
#endif




typedef char                CAM_Int8s;
typedef unsigned char       CAM_Int8u;
typedef short               CAM_Int16s;
typedef unsigned short      CAM_Int16u;
typedef int                 CAM_Int32s;
typedef unsigned int        CAM_Int32u;
typedef long long           CAM_Int64s;
typedef unsigned long long  CAM_Int64u;
typedef unsigned long long  CAM_Tick;
typedef int                 CAM_Bool;

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

#define CAM_TRUE            (CAM_Bool)(1)
#define CAM_FALSE           (CAM_Bool)(0)

#define CAM_STATISTIC_DISABLE   ((void*)(0xffffffff))

#define CAM_PORT_NONE       (-1)
#define CAM_PORT_PREVIEW    (0)
#define CAM_PORT_CAPTURE    (1)
#define CAM_PORT_ANY        (-1)




typedef struct {
    CAM_Int32s  iWidth;
    CAM_Int32s  iHeight;
} CAM_Size;

typedef struct {
    CAM_Int32s  iLeft;              // left boundary, the boundary itself is
                                    // included in the rectangle
    CAM_Int32s  iTop;               // top boundary, the boundary itself is
                                    // included in the rectangle
    CAM_Int32s  iWidth;             // width of the rect
    CAM_Int32s  iHeight;            // height of the rect
} CAM_Rect;

typedef struct{
    /* index in sensor resolution table */
    int         index;
    /* sensor output resolution */
    CAM_Int32u  iWidth;
    CAM_Int32u  iHeight;
    /* sensor optical resolution (before ISP) */
    CAM_Int32u  iRawWidth;
    CAM_Int32u  iRawHeight;
    /* maximum zoom-in factor */ 
    CAM_Int32u  iMaxZoomInQ16;
} CAM_SensorResolution;

typedef struct {
    int         iResIndex;
    CAM_Int32s  iFPS;
    CAM_Int32s  iMin;
    CAM_Int32s  iMax;
    CAM_Int32s  iMaxFPSAutoDeduce;
} CAM_EShutterLimit;





typedef enum {
    CAM_ERROR_NONE,                 

    CAM_ERROR_BADARGUMENT,          

    CAM_ERROR_NOTSUPPORTEDCOMMAND,

    CAM_ERROR_NOTSUPPORTEDPARAM,

    CAM_ERROR_ONWRONGSTATE,

    CAM_ERROR_NOTENOUGHRESOURCE,

    CAM_ERROR_BUFFERNOTRELEASED,

    CAM_ERROR_BUFFERNOTQUEUED,

    CAM_ERROR_BUFFERNOTDEQUEUED,

    CAM_ERROR_BUFFERNOTAVAILABLE,

    CAM_ERROR_OUTOFMEMORY,

    CAM_ERROR_SENSORINITFAILED,

    CAM_ERROR_OPFAILED,

    CAM_ERROR_STATISTICNOTENABLED,

    CAM_ERROR_INCONSISTENTSTATE,

    CAM_ERROR_BADPORTINDEX,

    CAM_ERROR_NOTIMPLEMENTED,

    CAM_ERROR_CONFIGNOTALLOWED

} CAM_Error;

typedef enum {
    CAM_IMGFMT_RGGB8 = 1000,
    CAM_IMGFMT_BGGR8,
    CAM_IMGFMT_GRBG8,
    CAM_IMGFMT_GBRG8,
    CAM_IMGFMT_RGGB10,
    CAM_IMGFMT_BGGR10,
    CAM_IMGFMT_GRBG10,
    CAM_IMGFMT_GBRG10,
    
    CAM_IMGFMT_RGB888 = 2000,
    CAM_IMGFMT_RGB444,
    CAM_IMGFMT_RGB555,
    CAM_IMGFMT_RGB565,
    CAM_IMGFMT_RGB666,
    
    CAM_IMGFMT_YCC444P = 3000,
    CAM_IMGFMT_YCC444I,
    CAM_IMGFMT_YCC422P,
    CAM_IMGFMT_YCbYCr,
    CAM_IMGFMT_CbYCrY,
    CAM_IMGFMT_YCrYCb,
    CAM_IMGFMT_CrYCbY,
    CAM_IMGFMT_YCC420P,
    
    CAM_IMGFMT_JPEG411 = 4000,
    CAM_IMGFMT_JPEG422,
} CAM_ImageFormat;

typedef enum {
    CAM_FLIPROTATE_NORMAL,
    CAM_FLIPROTATE_90L,
    CAM_FLIPROTATE_90R,
    CAM_FLIPROTATE_180,
    CAM_FLIPROTATE_HFLIP,                   // horizontal flip
    CAM_FLIPROTATE_VFLIP,                   // vertical flip
    CAM_FLIPROTATE_DFLIP,                   // diagonal flip
    CAM_FLIPROTATE_ADFLIP,                  // anti-diagonal flip
} CAM_FlipRotate;

typedef enum {
    CAM_WORKMODE_IDLE,
    CAM_WORKMODE_PREVIEW,
    CAM_WORKMODE_VIDEO,
    CAM_WORKMODE_STILL,
} CAM_WorkMode;

typedef enum {
    CAM_PHOTOMODE_AUTO,
    CAM_PHOTOMODE_MANUAL,
    CAM_PHOTOMODE_PROGRAM,
    CAM_PHOTOMODE_APERTUREPRIOR,
    CAM_PHOTOMODE_SHUTTERPRIOR,
    CAM_PHOTOMODE_CUSTOM,
    CAM_PHOTOMODE_PORTRAIT,
    CAM_PHOTOMODE_LANDSCAPE,
    CAM_PHOTOMODE_NIGHTPORTRAIT,
    CAM_PHOTOMODE_NIGHTSCENE,
    CAM_PHOTOMODE_CHILD,
    CAM_PHOTOMODE_INDOOR,
    CAM_PHOTOMODE_PLANTS,
    CAM_PHOTOMODE_SNOW,
    CAM_PHOTOMODE_BEACH,
    CAM_PHOTOMODE_FIREWORKS,
    CAM_PHOTOMODE_SUBMARINE
} CAM_PhotoMode;

typedef enum {
    CAM_WHITEBALANCEMODE_AUTO,
    CAM_WHITEBALANCEMODE_DAYLIGHT,
    CAM_WHITEBALANCEMODE_CLOUDY,
    CAM_WHITEBALANCEMODE_INCANDESCENT,
    CAM_WHITEBALANCEMODE_FLUORESCENT
} CAM_WhiteBalanceMode;

typedef enum {
    CAM_COLOREFFECT_OFF,
    CAM_COLOREFFECT_VIVID,
    CAM_COLOREFFECT_SEPIA,
    CAM_COLOREFFECT_BLACKWHITE,
    CAM_COLOREFFECT_NEGTIVE
} CAM_ColorEffect;

typedef enum {
    CAM_EXPOSUREMETERMODE_MEAN,
    CAM_EXPOSUREMETERMODE_CENTRALWEIGHTED,
    CAM_EXPOSUREMETERMODE_SPOT,
    CAM_EXPOSUREMETERMODE_MATRIX
} CAM_ExposureMeterMode;

typedef enum {
    CAM_ISO_AUTO,
    CAM_ISO_50,
    CAM_ISO_100,
    CAM_ISO_200,
    CAM_ISO_400,
    CAM_ISO_800,
    CAM_ISO_1600,
    CAM_ISO_3200
} CAM_ISOMode;

typedef enum {
    CAM_FOCUS_AUTO,
    CAM_FOCUS_MANUAL
} CAM_FocusMode;

typedef enum {
    CAM_STROBE_ON,
    CAM_STROBE_OFF,
    CAM_STROBE_AUTO,
} CAM_StrobeMode;

typedef enum {
    CAM_SHUTTERMODE_ELECTRONIC,     // Eletronical shutter:
                                    // * Can be used in video or still capture
                                    // * Mechanical shutter should be kept open.
                                    //   Exposure is electronically controlled.
                                    //   lines have different exposure start and
                                    //   stop time. Exposure duration is decided
                                    //   by "PCLK", "dummy pixels",
                                    //   "dummy lines" and "dummy frames".
                                    // * Image distortion could happen if there
                                    //   is motion
                                    // * Flicker could happen under man-made
                                    //   light source
                                    
    CAM_SHUTTERMODE_MECHANICAL,     // Half-Mechanical:
                                    // * Can be used in still capture only
                                    // * The whole frame have the same exposure
                                    //   start/stop and duration.
                                    // * Before shot, mechanical shutter is
                                    //   kept open, and sensor is put to reset
                                    //   state. When the exposure needs to start,
                                    //   the sensor is released, and a sync
                                    //   signal is send to mechanical shutter.
                                    //   When the exposure time is out, the
                                    //   mechanical shutter will close.
                                    // * The data is read out line by line. And
                                    //   during this period, mechanical shutter
                                    //   should kept closed.
                                    // * Can support "Bulb" mode by close the
                                    //   mechanical shutter with a external
                                    //   trigger.
                                    
    CAM_SHUTTERMODE_PUREMECHANICAL, // Fully-Mechanical
                                    // * Can be used in still capture only
                                    // * The whole frame have the same exposure
                                    //   start/stop and duration.
                                    // * Exposure start and stop are controled by
                                    //   mechanical
} CAM_ShutterMode;

typedef enum {
    CAM_FLASHTYPE_NONE,
    CAM_FLASHTYPE_LED,
    CAM_FLASHTYPE_XENON,
} CAM_FlashType;

typedef enum {
    CAM_FLASHMODE_OFF,              // turn off the flash light
    CAM_FLASHMODE_FRONTEDGE,        // flash begin right after the shutter is
                                    // opened
    CAM_FLASHMODE_BACKEDGE,         // flash finish right before the shutter is
                                    // closed
    CAM_FLASHMODE_TORCH,            // flash is manually turned on/off
} CAM_FlashMode;

typedef enum {

    //Statistic Method                      // Param
    //                                      // Stat Result
    //                                      // Comments

    // IPP statistic method
    CAM_STATISTIC_ARBWINWEIGHTEDLUM,        // CAM_ArbWinWeightedParam *pParam
                                            // CAM_Int16u *pLumAvg
                                            // weighted luminance arverge

    CAM_STATISTIC_ARBWINWEIGHTEDRGBG,       // CAM_ArbWinWeightedParam *pParam
                                            // CAM_Int16u rg_bg_ratio[2]
                                            // weighted r/g, b/g ratios
                                            
    // Smart sensor statisic method
    CAM_STATISTIC_4X4WEIGHTEDLUMAVG,        // CAM_4X4WeightedLumParam *pParam
                                            // CAM_Int16u *pLumAvg
                                            // luminance average
                                            
    // ...
    CAM_STATISTIC_FRAMERGBHISTOGRAM,        // NULL
                                            // CAM_Int32s histogram[3][32]
                                            // 32 bins histograms for R, G, B
                                            // channels
                                            
    CAM_STATISTIC_FRAMELUMHISTOGRAM,        // NULL
                                            // CAM_Int32s histogram[32]
                                            // 32 bins histogram for luminance,
                                            // luminance can be defined by
                                            // statistic unit
                                            
    // Smart CI statistic method
    CAM_STATISTIC_8Z64P,                    // CAM_8Z64PParam
                                            // CAM_8Z64PResult *pResult
                                            // Statistic R/G/B average and Y
                                            // sharpness in 8 zones. Each zone
                                            // is composed of patches. The whole
                                            // frame are evenly divided into 8x8
                                            // totally 64 patches. Each patch
                                            // can be config to belong to one
                                            // zone. The statistic result is
                                            // updated every frame.
} CAM_StatisticMethod;

// Parameter structure for arbitrary window weighted brightness
typedef struct {
    int iWindowNum;
    CAM_Rect *pWindows;
    CAM_Int32s *pWeights;
} CAM_ArbWinWeightedParam;

// Parameter structure for window rgb/lum sum
typedef struct {
    CAM_Int32s iLeft;
    CAM_Int32s iTop;
    CAM_Int32s iWidth;
    CAM_Int32s iHeight;
    CAM_Int32s iWeight[16];
} CAM_4X4WeightedLumParam;

// Parameter structure for 8Z64P statistic method
typedef struct {
    CAM_Int8u   iRegionIndex[64];
//  CAM_*****   ****************;           // filter for <r/g-b/g> color space
} CAM_8Z64PParam;

// Result structure for 8Z64P statistic method
typedef struct {
    CAM_Int32s  iSumR[8];
    CAM_Int32s  iSumG[8];
    CAM_Int32s  iSumB[8];
    CAM_Int32s  iCount[8];
} CAM_8Z64PResult;

// Parameter structure for SET_STATISTIC_PARAMETER and GET_STATISTIC_RESULT

typedef struct {
    CAM_StatisticMethod eMethod;
    void                *pData;     // content of pData depend on the eMethod
} CAM_StatisticData;


// Image buffer header
typedef struct {
    CAM_ImageFormat         eFormat;
    CAM_Int32s              iWidth;
    CAM_Int32s              iHeight;
    CAM_Int32s              iStep[3];
    CAM_Int32s              iAllocLen[3];
    CAM_Int32s              iFilledLen[3];
    CAM_Int32s              iOffset[3];
    CAM_Int8u               *pBuffer[3];
    CAM_Int32s              iStartLine;
    CAM_Tick                iTick;
    CAM_Int32u              iFlag;
    int                     iIndex;    // index of the buffer in streaming buffer queue
} CAM_ImageBuffer;

// Buffer requirement for streaming buffer queue
typedef struct {
    CAM_ImageFormat         eFormat;
    CAM_Int32s              iWidth;    // image width in pixels
    CAM_Int32s              iHeight;   // image height in pixels
    CAM_Int32s              iStep[3];  // image buffer's row stride in bytes
    CAM_Int32s              iBufLen[3];
    CAM_Int32s              iCount;
    CAM_Int32s              iAlignment;
    CAM_Int32s              iSliceHeight;
    CAM_Bool                bPhysicalContiguous;

    CAM_ImageBuffer         *pBufArray;
} CAM_ImageBufferReq;


/* 
 * Camera Engine entry API definitions
 */

typedef void*   CAM_DeviceHandle;
typedef int     CAM_Command;
typedef void*   CAM_Parameter;


typedef CAM_Error (*_CAM_OpenFunc)(void **ppDeviceData);

typedef CAM_Error (*_CAM_CloseFunc)(void **ppDeviceData);

typedef CAM_Error (*_CAM_CommandFunc)(void *pDeviceData,
                                      CAM_Command cmd,
                                      int param1,
                                      CAM_Parameter param2);

typedef struct _CAM_DriverEntry {
    const char          *pName;
    _CAM_OpenFunc       pOpen;
    _CAM_CloseFunc      pClose;
    _CAM_CommandFunc    pCommand;
} CAM_DriverEntry;


CAM_Error CAM_GetHandle(const CAM_DriverEntry *pDriverEntry,
                        CAM_DeviceHandle *pHandle);

CAM_Error CAM_FreeHandle(CAM_DeviceHandle *pHandle);

CAM_Error CAM_SendCommand(CAM_DeviceHandle handle, 
                          CAM_Command cmd,
                          int param1,
                          CAM_Parameter param2);


#ifdef __cplusplus
}
#endif

#endif  // _CAM_GEN_H_


