/******************************************************************************
//(C) Copyright [2009] Marvell International Ltd.
//All Rights Reserved
******************************************************************************/


#ifndef _CAM_SUBCOMP_H_
#define _CAM_SUBCOMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "CamGen.h"


extern CAM_DriverEntry entry_subcomp;


/*------------------------------------------------------------------------------
--  Commands for camera sub-components (the hardware abstraction layer -- L2)
------------------------------------------------------------------------------*/

typedef enum {
////////////////////////////////////////////////////////////////////////////////
//  CAM_Error CAM_SendCommand(handle, Command, DeviceID, Parameter);
//
//  Command                             Parameter
//                                      [MANDATORY / OPTIONAL / RESERVED]
//                                      Comments
//
//  Notes:
//  1. Current version is ONLY designed for Smart Sensors, which have embedded
//     hardware ISP inside.   
//  2. There are 3 types of L2 commands:
//       MANDATORY: must be implemented;
//       OPTIONAL:  if not implemeted, return CAM_ERROR_NOTIMPLEMENTED
//                  in subcomp_command();
//       RESERVED:  reserved for future use, i.e. haven't been used
//                  by IPP Camera Engine yet.
//  3. Some MANDATORY are mandatory only for still capture, not for video
//     recording. This will be denoted in comments of those commands.
//  4. In the comments of each commands, caller indicates L1.
////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
    // Sensor commands
    // These commands should provided by sensor driver which may be implemented
    // by sensor vendor
    ////////////////////////////////////////////////////////////////////////////
                                        
    CAM_SENSOR_GET_RESOLUTION,          // [IN/OUT] CAM_SensorResolution *pResolution
                                        // [MANDATORY]
                                        // Caller sets the "index" field before
                                        // calling, L2 needs to fill other fields
                                        // on return.
                                        // Smart sensor usually has two types of
                                        // resolutions, optical resolution and
                                        // output resolution. Optical resolution
                                        // depends on the optical sensor array.
                                        // While output resolution is scaled from
                                        // optical resolution by hardware ISP.                                      

    CAM_SENSOR_SET_FPS,                 // [IN] CAM_Int32s *pFrameRateQ16
                                        // [OPTIONAL]
                                        // Set sensor output frame rate, which in
                                        // Q16 format.
                                        // Under certain resolution, sensor might
                                        // have a limited range of supportd frame
                                        // rates. If the required fps is out of
                                        // range, L2 returns CAM_ERROR_NOTSUPPORTEDPARAM
                                        // to notify the caller.
                                                                                
    CAM_SENSOR_SET_NIGHTMODEFPSLIMIT,   // [IN] CAM_Int32s *pFrameRateQ16
                                        // [OPTIONAL]
                                        // Set the frame rate limit under night
                                        // mode.
                                        
    CAM_SENSOR_SET_SHUTTERMODE,         // [IN] CAM_ShutterMode *pShutterMode
                                        // [RESERVED]
                                        // Chooose either electronic shutter
                                        // or mechnical shutter. 

    CAM_SENSOR_SET_ESHUTTERSPEEDINDEX,  // [IN] int *pEShutterSpeedIndex
                                        // [RESERVED]
                                        // Set electronic shutter speed.
    
    CAM_SENSOR_SET_ANALOGGAININDEX,     // [IN] int *pAnalogGainIndex
                                        // [RESERVED]
                                        // Set analog gain index.
                                                                                
    CAM_SENSOR_SET_DIGITALGAININDEX,    // [IN] int *pDigitalGainIndex
                                        // [RESERVED]
                                        // Set digital gain index.
                                        
   
    ////////////////////////////////////////////////////////////////////////////
    // Image Signal Processing commands
    ////////////////////////////////////////////////////////////////////////////
   
    // exposure / white balance / focusing - auto control
    CAM_ISP_ENABLE_AE,                  // [IN] CAM_Bool *pEnable
                                        // [MANDATORY]
                                        // Enable or disable automatic exposure
                                        // control in smart sensor.
                                        
    CAM_ISP_ENABLE_AWB,                 // [IN] CAM_Bool *pEnable
                                        // [MANDATORY]
                                        // Enable or disable automatic white
                                        // balance control in smart sensor.
                                        
    CAM_ISP_START_AF,                   // [NULL]
                                        // [OPTIONAL]
                                        // Trigger the start of auto focusing.
                                        // AF support depends on each camera module
                                                                                
    CAM_ISP_CANCEL_AF,                  // [NULL]
                                        // [OPTIONAL]
                                        // Cancel auto focusing process. 
                                                                                
    CAM_ISP_CHECK_AF,                   // [OUT] CAM_Int16u *pAFStatus
                                        // [OPTIONAL]
                                        // Get auto focusing status.
                                        
    CAM_ISP_SAVE_3A,                    // [NULL]
                                        // [MANDATORY]
                                        // ONLY for still capture. 
                                        // Caller sends this command before switch
                                        // from preview to still capture, and
                                        // L2 needs to save 3A control results,
                                        // by reading out and save certain registers.

    CAM_ISP_RESTORE_3A,                 // [NULL]
                                        // [MANDATORY]
                                        // ONLY for still capture.
                                        // Caller sends this command right after
                                        // entering still captue stage, and L2 needs
                                        // to restore and apply saved 3A settings to
                                        // certain registers.
                                        
    // exposure / white balance / focusing - statistic
    CAM_ISP_SET_STATISTIC,              // [IN] CAM_StatisticMethod *pStatisticMethod
                                        // [RESERVED]
                                        // Set statistic method.
                                        
    CAM_ISP_GET_STATISTIC,              // [OUT] CAM_StatisticData *pResult
                                        // [RESERVED]
                                        // Get statistic result.
    
    // image processing
    CAM_ISP_SET_ISO,                    // [IN] CAM_ISOMode *pISO
                                        // [OPTIONAL]
                                        // Set ISO mode. Refer to the definition
                                        // of CAM_ISOMode. 

    CAM_ISP_SET_EVCOMPENSATION,         // [IN] int *pEVCompensationIndex
                                        // [OPTIONAL]
                                        // Set EV level. 0 is reserved for auto mode.

    CAM_ISP_SET_COLOREFFECT,            // [IN] CAM_ColorEffect *pColorEffect
                                        // [OPTIONAL]
                                        // Set color effect. Refer to the
                                        // definition of CAM_ColorEffect.

    CAM_ISP_SET_WHITEBALANCEMODE,       // [IN] CAM_WhiteBalanceMode *pWhiteBalance
                                        // [OPTIONAL]
                                        // Set white balance mode. Refer to the
                                        // definition of CAM_WhiteBalanceMode. 

    CAM_ISP_SET_SATURATION,             // [IN] int *pSaturationIndex
                                        // [OPTIONAL]
                                        // Set saturation level. 0 is reserved 
                                        // for auto mode.

    CAM_ISP_SET_CONTRAST,               // [IN] int *pContrastIndex
                                        // [OPTIONAL]
                                        // Set contrast level. 0 is reserved for
                                        // auto mode.

    CAM_ISP_SET_EXPOSUREMEASUREMODE,    // [IN] CAM_ExposureMeterMode *pExposureMeasure
                                        // [OPTIONAL]
                                        // Set exposure measurement mode. Refer
                                        // to the definition of CAM_ExposureMeterMode.

    CAM_ISP_SET_NIGHTMODE,              // [IN] CAM_Bool *pEnable
                                        // [OPTIONAL]
                                        // Enable or disable night mode.

    CAM_ISP_SET_BANDFILTER,             // [IN] int *pBandFilterIndex
                                        // [OPTIONAL] 
                                        // Set 50/60Hz band filter. 0 is reserved 
                                        // for auto mode.
    
    CAM_ISP_SET_DENOISE,                // [IN] int *pDenoiseLevel
                                        // [OPTIONAL]
                                        // Set denoise level.

    CAM_ISP_SET_EDGEENHANCEMENT,        // [IN] int *pEdgeEnhanceLevel
                                        // [OPTIONAL]
                                        // Set edge enhancement level.

    CAM_ISP_SET_SCALEFACTORINDEX,       // [IN] int *pDigZoomFactorIndex
                                        // [OPTIONAL]
                                        // Set the digital zoom factor.
                                        
    CAM_ISP_SET_FLIPROTATE,             // [IN] CAM_FlipRotate *pFlipRotate
                                        // [OPTIONAL]
                                        // Set flip rotate mode. Refer to the 
                                        // definition of CAM_FlipRotate.

    CAM_ISP_SET_CROPPINGWINDOW,         // [IN] CAM_Rect *pRect
                                        // [RESERVED]
                                        // Set cropping window size and position.
                                        
     CAM_ISP_SET_BLACKLEVEL,            // [IN] CAM_Int16s iBLC[4]
                                        // [RESERVED]
                                        // Index 0~3 are for R, Gr, Gb, B
                                        
    CAM_ISP_SET_CHANNELGAIN,            // [IN] CAM_Int16s iChannelGain[3]
                                        // [RESERVED]
                                        // Index 0~2 are for R, G, B
                                        
    CAM_ISP_SET_COLORCORRECTION,        // [IN] CAM_Int16s iMatrixQ8[3][4]
                                        // [RESERVED]
                                        // Set color correction matrix.
                                                                
    CAM_ISP_SET_RGB2YUV,                // [IN] CAM_Int16s iMatrixQ8[3][4]
                                        // [RESERVED]
                                        // Set sRGB to YUV conversion matrix.
                                        
    CAM_ISP_SET_RGBTONECURVE,           // [IN] CAM_Int16s *pToneCurve[3]
                                        // [RESERVED]
                                        // Set gamma correction and tone
                                        // mapping.
                                        
    CAM_ISP_SET_LTONECURVE,             // [IN] CAM_Int16s *pToneCurve
                                        // [RESERVED]
                                        // Used for tone mapping on luminance
 
    // Image reconstruction parameters
    CAM_ISP_SET_CONFIGFILE,             // [IN] CAM_Int8u *pBinary
                                        // [RESERVED]
                                        // set the ISP configuration file which
                                        // is generated during ISP tuning. the
                                        // content depend on the ISP vendor. it
                                        // might contains parameters for dead
                                        // pixel, lens shading, color correction
                                        // matrix, etc.
                                        
    // Flicker detection
    CAM_ISP_ENABLE_FLICKERDETECTION,    // [IN] CAM_Bool *pEnable
                                        // [RESERVED]
                                        // enable/disable the flicker detection.
                                        // flicker happens only when line
                                        // exposure mode is used.
                                        
    CAM_ISP_GET_FLICKERFREQUENCY,       // [IN] int *pFrequency
                                        // [RESERVED]
                                        // return the luminance variation
                                        // frequency. The result could be 0, 50
                                        // or 60. It returns error when flicker
                                        // detection is disabled


    ////////////////////////////////////////////////////////////////////////////
    // Lens parameters (include zoom, focus, aperture and mechanical shutter)
    ////////////////////////////////////////////////////////////////////////////
    CAM_LENS_GET_PROPERTY,              // [OUT] *pProperty
                                        // [RESERVED]
                                        // Get the 
                                        
    CAM_LENS_SET_MSHUTTERMANUAL,        // [IN] int *pOpen
                                        // [RESERVED]
                                        // Manually open or close the m-shutter
                                        
    CAM_LENS_SET_MSHUTTERAUTO,          // [IN] int iMShutterSpeedIndex
                                        // [RESERVED]
                                        // Setup for automatic shutter open
                                        
    CAM_LENS_SET_ZOOMFACTORINDEX,       // [IN] int iZoomFactorIndex
                                        // [RESERVED]
                                        
    CAM_LENS_SET_FNUMBERINDEX,          // [IN] int *pFNumberIndex
                                        // [RESERVED]
                                        
    CAM_LENS_SET_FOCUSPOSITON,          // [IN] unsigned long *pFocusPos
                                        // [RESERVED]

    ////////////////////////////////////////////////////////////////////////////
    // Flash parameters
    ////////////////////////////////////////////////////////////////////////////
    CAM_FLASH_GET_PROPERTY,             // [OUT] *pProperty;
                                        // [RESERVED]
                                        // Get the "guide number" and the "color
                                        // temperature" of the strobe.
                                        
    CAM_FLASH_SET_FLASHMODE,            // [IN] CAM_FlashMode *pFlashMode
                                        // [RESERVED]
                                        // can be "torch", "front edge" or "back
                                        // edge"
                                        
    CAM_FLASH_SET_FLASHRECHARGE,        // [NULL]
                                        // [RESERVED]
                                        // command the flash light to start
                                        // recharged
                                        
    CAM_FLASH_GET_FLASHRECHARGE,        // [OUT] CAM_Bool *pRecharged
                                        // [RESERVED]
                                        // check if the flash light has finished
                                        // recharge
                                        
    CAM_FLASH_SET_FLASHDURATION,        // [IN] CAM_Int64u *pDuration
                                        // [RESERVED]
                                        // given the guide number, ambient light
                                        // and distance to subject, the
                                        // duration can control the subject's
                                        // brightness in the output image
} CAM_SubCameraCommand;


#ifdef __cplusplus
}
#endif

#endif  // _CAM_SUBCOMP_H_

