/******************************************************************************
//(C) Copyright [2009] Marvell International Ltd.
//All Rights Reserved
******************************************************************************/


#ifndef _CAM_RAWCAM_H_
#define _CAM_RAWCAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "CamGen.h"


extern CAM_DriverEntry entry_rawcamera_smt;


// Reference Clock Callback
typedef CAM_Tick (*ReferenceClockFunc)(void);


/*------------------------------------------------------------------------------
--  Commands for Raw Camera (the core layer -- L1)
------------------------------------------------------------------------------*/

typedef enum {
////////////////////////////////////////////////////////////////////////////////
//  Command                             Parameter
//                                      [MANDATORY / OPTIONAL]
//                                      Comments
////////////////////////////////////////////////////////////////////////////////

    // State transition operations
    CAM_RAWCAM_SET_WORKMODE,            // [IN] CAM_WorkMode *pWorkMode
                                        // [MANDATORY]
                                        // four modes: IDLE, PREVIEW, VIDEO, STILL
                                        // EFFECT ON CAMERA
                                        
    CAM_RAWCAM_SLEEP,                   // NULL
                                        // [MANDATORY]
                                        // change from the "running" state to
                                        // "sleeping" state which means the
                                        // data flow stops and the camera
                                        // enters a low power state
                                        // EFFECT ON CAMERA
                                        
    CAM_RAWCAM_WAKEUP,                  // NULL
                                        // [MANDATORY]
                                        // change from the "sleeping" state to
                                        // "running" state
                                        // EFFECT ON CAMERA
                                        
    // Buffer management and data flow
    CAM_RAWCAM_GET_BUFREQ,              // [OUT] CAM_ImageBufferReq *pBufReq
                                        // [MANDATORY]
                                        // get the buffer requirement of raw
                                        // camera. note that the buffer
                                        // requirement might change according to
                                        // current size/rotation settings
                                        // EFFECT ON PORT

    CAM_RAWCAM_ALLOC_BUF,               // [INOUT] CAM_ImageBufferReq *pBufReq
                                        // [MANDATORY]
                                        // allocate image buffer array according
                                        // to the given buffer requirement, fill
                                        // the buffer array pointer into the
                                        // image buffer reqirement structure.
                                        // the allocated array of buffers will
                                        // be used at the running state. the
                                        // previously allocated / used buffers
                                        // must be freed / unused before call
                                        // this command
                                        // EFFECT ON PORT

    CAM_RAWCAM_FREE_BUF,                // [INOUT] CAM_ImageBufferReq *pBufReq
                                        // [MANDATORY]
                                        // free port buffer, used in pair with
                                        // CAM_RAWCAM_ALLOC_BUFFER.
                                        // EFFECT ON PORT

    CAM_RAWCAM_USE_BUF,                 // [IN] CAM_ImageBufferReq *pBufReq
                                        // [MANDATORY]
                                        // use the buffer array in the input
                                        // structure at the running state.
                                        // the previously allocated / used
                                        // buffers must be freed / unused before
                                        // call this command.
                                        // EFFECT ON PORT

    CAM_RAWCAM_UNUSE_BUF,               // [IN] CAM_ImageBufferReq *pBufReq
                                        // [MANDATORY]
                                        // buffer release command corresponds to
                                        // CAM_RAWCAM_USE_BUF
                                        // EFFECT ON PORT

    CAM_RAWCAM_QUEUE_BUF,               // [IN] CAM_ImageBuffer *pBuf
                                        // [MANDATORY]
                                        // 
                                        // EFFECT ON PORT

    CAM_RAWCAM_DEQUEUE_BUF,             // [OUT] CAM_ImageBuffer **ppBuf
                                        // [MANDATORY]
                                        // Get the next filled buffer from
                                        // driver. It will block the call if
                                        // the next frame is not ready yet. If
                                        // the image data is failed to fill or
                                        // the image data is filled with defects
                                        // an error code will be returned.
                                        // After change from "running" to
                                        // "config" state, all previously queued
                                        // buffers are all automatically
                                        // released and should not call DQBUF to
                                        // retrieve them anymore
                                        // EFFECT ON PORT

    CAM_RAWCAM_FLUSH_BUF,               // NULL
                                        // [MANDATORY]
                                        // this call force DMA to be cancelled
                                        // and all the queued buffers to be
                                        // released
                                        // EFFECT ON PORT

    CAM_RAWCAM_SET_REFCLOCK,            // ReferenceClockFunc GetTick
                                        // [MANDATORY]
                                        // set the callback function to get the
                                        // current time. the camera driver
                                        // should call this callback whenever
                                        // a frame DMA is about to start and
                                        // use the returned value to fill the
                                        // "iTick" in buffer structure
                                        // EFFECT ON CAMERA

    // configuration file
    CAM_RAWCAM_SET_CONFIGFILE,          // [IN] CAM_Int8u *pBinary
                                        // [MANDATORY]
                                        // set the configuration file which
                                        // is generated during raw camera tuning
                                        // The content depend on the raw camera
                                        // vendor. it might contains parameters
                                        // for dead pixel, lens shading, color
                                        // correction matrix, etc.
                                        // EFFECT ON CAMERA

    // Basic set/get
    CAM_RAWCAM_SET_SENSORID,            // [IN] int *pSensorID
                                        // [MANDATORY]
                                        // set ID of which sensor to use
                                        // EFFECT ON CAMERA
                                        
    CAM_RAWCAM_GET_CAPS,                // [OUT] CAM_RawCameraCapability *pCaps
                                        // [MANDATORY]
                                        // this command is TBD
                                        // EFFECT ON CAMERA
                                        
    CAM_RAWCAM_SET_BURSTCOUNT,          // [IN] int iBurstCount
                                        // [MANDATORY]
                                        // set the buffer burst count.
                                        // EFFECT ON PORT
                                        
    CAM_RAWCAM_SET_FPS,                 // [INOUT] CAM_Int32s *pFrameRateQ16
                                        // [MANDATORY]
                                        // set the frame rate.
                                        // EFFECT ON PORT
                                        
    CAM_RAWCAM_SET_NIGHTMODEFPSLIMIT,   // [IN] CAM_Int32s *pFrameRateQ16
                                        // [MANDATORY]
                                        // set the frame rate.
                                        // EFFECT ON PORT
                                        
    CAM_RAWCAM_SET_FORMAT,              // [IN] CAM_ImageFormat *pFormat
                                        // [MANDATORY]
                                        // set the output image format
                                        // EFFECT ON PORT
                                        
    CAM_RAWCAM_SET_RESOLUTIONINDEX,     // [IN] int *pResolutionIndex
                                        // [MANDATORY]
                                        // the index to the frame modes. after
                                        // change resolution, cropping window
                                        // will automatically set to the full
                                        // frame.
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_SET_SCALEFACTORINDEX,    // [IN] int *pScaleFactorIndex
                                        // [MANDATORY]
                                        // set the digital zoom factor
                                        // EFFECT ON PORT
                                        
    CAM_RAWCAM_SET_CROPPINGWINDOW,      // [IN] CAM_Rect *pRect
                                        // [MANDATORY]
                                        // change the size and the position of
                                        // the cropping window
                                        // EFFECT ON PORT
                                        
    CAM_RAWCAM_SET_PREVIEWSIZE,         // [IN] CAM_Size *pSize
                                        // [MANDATORY]
                                        // change the size and the position of
                                        // the cropping window
                                        // EFFECT ON PORT

    CAM_RAWCAM_SET_PREVIEWAUTOCROP,     // [IN] CAM_Bool *pBool
                                        // [OPTIONAL]
                                        // automatically crop preview frames if
                                        // sensor and screen's width-height-ratio
                                        // are mismatch
                                        // CAM_TRUE:  enable
                                        // CAM_FALSE: disable
                                        // EFFECT ON PREVIEW PORT
                                        
    CAM_RAWCAM_SET_FLIPROTATE,          // [IN] CAM_FlipRotate *pFlipRotate
                                        // [MANDATORY]
                                        // rotate or flip the image. 
                                        // EFFECT ON PORT

    // Exposure parameters
    CAM_RAWCAM_SET_SHUTTERMODE,         // const CAM_ShutterMode *pShutterMode
                                        // [MANDATORY]
                                        // "electronic shutter" or "mechanical
                                        // shutter"
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_QUERY_ESHUTTERLIMIT,     // [INOUT] CAM_EShutterLimit *pLimit
                                        // [MANDATORY]
                                        // query the E-shutter limits corresponds
                                        // to the given fps and resolution.
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_QUERY_ESHUTTER,          // [INOUT] CAM_Int32s *pEShutterSpeed
                                        // [MANDATORY]
                                        // query the closest e-shutter speed
                                        // to the given shutter time

    CAM_RAWCAM_SET_ESHUTTERSPEED,       // [IN] CAM_Int32s *pShutterSpeed
                                        // [MANDATORY]
                                        // set the electronical shutter speed.
                                        // unit: MICROSECOND
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_SET_MSHUTTERSPEEDINDEX,  // [IN] int *pShutterSpeedIndex
                                        // [MANDATORY]
                                        // set the shutter speed by index. To
                                        // support bulb mode, frame exposure
                                        // should be used and the shutter speed
                                        // index should be CAM_BULB_MODE.
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_STOP_BULBEXPOSURE,       // NULL
                                        // [MANDATORY]
                                        // stop the bulb mode exposure. This
                                        // will close the mechanical shutter,
                                        // then the image processing and output
                                        // is not in use, or not in BULB mode,
                                        // it will return error.
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_SET_ANALOGGAININDEX,     // [IN] int *pAnalogGainIndex
                                        // [MANDATORY]
                                        // set the analog gain index
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_SET_DIGITALGAIN,         // [IN] CAM_Int16s iGain
                                        // [MANDATORY]
                                        // 
                                        // EFFECT ON CAMERA

    // Lens control parameters
    CAM_RAWCAM_SET_OPTZOOMFACTORINDEX,  // [IN] int *pOptZoomFactorIndex
                                        // [MANDATORY]
                                        // set the optical zoom factor
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_SET_FNUMBERINDEX,        // [IN] int *pFNumberIndex
                                        // [MANDATORY]
                                        // set the f-number to change the
                                        // aperture size
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_SET_FOCUSPOSITON,        // [IN] CAM_Int32u *pFocusPos
                                        // [MANDATORY]
                                        // set the focus position
                                        // EFFECT ON CAMERA

    // Flash control parameters
    CAM_RAWCAM_SET_FLASHMODE,           // [IN] CAM_FlashMode *pFlashMode
                                        // [MANDATORY]
                                        // can be "off", "torch", "front edge"
                                        // or "back edge"
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_SET_FLASHDURATION,       // [IN] CAM_Int64u *pDuration
                                        // [MANDATORY]
                                        // given the guide number, ambient light
                                        // and distance to subject, the
                                        // duration can control the subject's
                                        // brightness in the output image
                                        // EFFECT ON CAMERA

    // Color parameters
    CAM_RAWCAM_SET_CHANNELGAIN,         // [IN] CAM_Int16s iChannelGainQ8[3]
                                        // [MANDATORY]
                                        // Index 0~3 are for R, G, B
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_SET_COLORCORRECTION,     // [IN] CAM_Int16s iMatrixQ8[3][4]
                                        // [MANDATORY]
                                        // applied to cRGB or RGB' depend on ISP
                                        // EFFECT ON CAMERA

    // Tone mapping parameters
    CAM_RAWCAM_SET_RGBTONECURVE,        // [IN] CAM_Int8u *pToneCurve[3]
                                        // [MANDATORY]
                                        // pointer array with 3 elements point
                                        // to 64 piece wise gamma curves (have
                                        // 65 points). used for gamma correction
                                        // and global tone mapping
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_SET_LTONECURVE,          // [IN] CAM_Int8u *pToneCurve
                                        // [OPTIONAL]
                                        // pointer to a 64 piece wise gamma
                                        // curve (has 65 points). used for
                                        // luminance based global tone mapping
                                        // EFFECT ON CAMERA

    // Image enhancement and post processing
    CAM_RAWCAM_SET_DENOISE,             // [IN] int *pLevel
                                        // [OPTIONAL]
                                        // Level 0~7, means from no denoise to
                                        // the strongest denoise. Strong denoise
                                        // might cause higher time/power
                                        // consumption.
                                        // EFFECT ON PORT

    CAM_RAWCAM_SET_EDGEENHANCEMENT,     // [IN] int *pLevel
                                        // [OPTIONAL]
                                        // Level 0~7, means from no edge
                                        // enhancement to the strongest edge
                                        // enhancement.
                                        // EFFECT ON PORT

    // Statistic values
    CAM_RAWCAM_SET_STATISTIC,           // [IN] CAM_StatisticData *pParameter
                                        // [MANDATORY]
                                        // if the given statistic method is not
                                        // enabled, it will firstly enable, then
                                        // set the parameter. if the given
                                        // statistic method has been enabled, it
                                        // will only change the parameter for
                                        // the given statistic method. if the
                                        // "pData" is CAM_STATISTIC_DISABLE, the
                                        // corresponding statistic method will
                                        // be disabled
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_GET_STATISTIC,           // [OUT] CAM_StatisticData *pResult
                                        // [MANDATORY]
                                        // get the statistic result for the
                                        // given statistic method
                                        // EFFECT ON CAMERA

    // Flicker detection
    CAM_RAWCAM_ENABLE_FLICKERDETECTION, // [IN] CAM_Bool *pEnable
                                        // [OPTIONAL]
                                        // enable the flicker detection. flicker
                                        // happens only when line exposure mode
                                        // is used.
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_GET_FLICKERFREQUENCY,    // [OUT] int *pFrequency
                                        // [OPTIONAL]
                                        // return the luminance variation
                                        // frequency. The result could be 0, 50
                                        // or 60. It returns error when flicker
                                        // detection is disabled
                                        // EFFECT ON CAMERA

    // exposure / white balance / focusing - auto control provided by the
    // internal ISP
    CAM_RAWCAM_ENABLE_AE,               // [IN] CAM_Bool *pEnable
                                        // [OPTIONAL]
                                        // 
                                        // EFFECT ON PORT

    CAM_RAWCAM_ENABLE_AWB,              // [IN] CAM_Bool *pEnable
                                        // [OPTIONAL]
                                        // 
                                        // EFFECT ON CAMERA

    CAM_RAWCAM_START_AF,                // [NULL]
                                        // [OPTIONAL]
                                        //
    CAM_RAWCAM_CANCEL_AF,               // [NULL]
                                        // [OPTIONAL]
                                        //
    CAM_RAWCAM_CHECK_AF,                // [OUT] CAM_Int16u *pAFStatus
                                        // [OPTIONAL]

    /* Photo control commands */
    CAM_RAWCAM_SET_PHOTOMODE,           // [IN] CAM_PhotoMode *pPhotoMode

    CAM_RAWCAM_SET_ISO,                 // [IN] CAM_ISOMode *pISO

    CAM_RAWCAM_SET_EVCOMPENSATION,      // [IN] int *pEVCompensationIndex

    CAM_RAWCAM_SET_SHUTTERSPEEDINDEX,   // [IN] int *pShutterSpeedIndex

    CAM_RAWCAM_SET_FNUMINDEX,           // [IN] int *pFNumIndex

    CAM_RAWCAM_SET_COLOREFFECT,         // [IN] CAM_ColorEffect *pColorEffect

    CAM_RAWCAM_SET_WHITEBALANCEMODE,    // [IN] CAM_WhiteBalanceMode *pWhiteBalance

    CAM_RAWCAM_SET_SATURATION,          // [IN] int *pSaturationIndex

    CAM_RAWCAM_SET_CONTRAST,            // [IN] int *pContrastIndex

    CAM_RAWCAM_SET_EXPOSUREMEASUREMODE, // [IN] int *pExposureMeasureModeIndex

    CAM_RAWCAM_SET_NIGHTMODE,           // [IN] CAM_Bool *pEnable

    CAM_RAWCAM_SET_BANDFILTER,          // [IN] int *pBandFilterIndex       
                          
} CAM_RawCameraCommand;


#ifdef __cplusplus
}
#endif

#endif  // _CAM_RAWCAM_H_

