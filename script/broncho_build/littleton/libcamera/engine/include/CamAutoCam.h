/******************************************************************************
//(C) Copyright [2009] Marvell International Ltd.
//All Rights Reserved
******************************************************************************/

#ifndef _CAM_AUTOCAM_H_
#define _CAM_AUTOCAM_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "CamGen.h"


extern CAM_DriverEntry entry_autocam;


typedef enum {
    CAM_USECASE_VIDEO,                      // video recording
    CAM_USECASE_STILL                       // still capture
} CAM_UseCase;


/*------------------------------------------------------------------------------
--  Commands for Auto Camera (the application adaptation layer -- L0)
------------------------------------------------------------------------------*/

typedef enum {
////////////////////////////////////////////////////////////////////////////////
//  Command                                 Parameter
//                                          Comments
////////////////////////////////////////////////////////////////////////////////
    // State machine control commands
    CAM_AUTOCAM_SET_USECASE,                // [IN] CAM_UseCase *pUseCase
                                            // EFFECT ON CAMERA
                                                                                         
    CAM_AUTOCAM_CAPTURE,                    // [NULL]
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_PREVIEW,                    // [NULL]
                                            // EFFECT ON CAMERA
                                            
    CAM_AUTOCAM_STANDBY,                    // [NULL]
                                            // EFFECT ON CAMERA
                                          
    CAM_AUTOCAM_IDLE,                       // [NULL]
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_SENSORID,               // [IN] int *pSensorID
                                            // specify which sensor to use.
                                            // MUST send this command before streaming.
                                            // EFFECT ON CAMERA
    
    // Streaming buffer control commands
    CAM_AUTOCAM_GET_BUFREQ,                 // [OUT] CAM_ImageBufferReq *pBufReq
                                            // get buffer requirement for streaming
                                            // EFFECT ON PORT
                                            
    CAM_AUTOCAM_ALLOC_BUF,                  // [IN/OUT] CAM_ImageBufferReq *pBufReq
                                            // allocate image buffer array according
                                            // to specified buffer requirement.
                                            // On return, the buffer array pointer is filled
                                            // into the image buffer requirement structure.
                                            // Before calling, the previously allocated/used
                                            // buffers must have been freed/unused.
                                            // EFFECT ON PORT
                                            
    CAM_AUTOCAM_FREE_BUF,                   // [IN/OUT] CAM_ImageBufferReq *pBufReq                                            // [MANDATORY]
                                            // Free the buffer array previously allocated
                                            // by CAM_AUTOCAM_ALLOC_BUFFER.
                                            // EFFECT ON PORT
                                            
    CAM_AUTOCAM_USE_BUF,                    // [IN] CAM_ImageBufferReq *pBufReq
                                            // Use the application-provided buffer array
                                            // according to specified buffer requirement.
                                            // Before calling, the previously allocated/used
                                            // buffers must have been freed/unused. 
                                            // EFFECT ON PORT

    CAM_AUTOCAM_UNUSE_BUF,                  // [IN] CAM_ImageBufferReq *pBufReq
                                            // Free the buffer array previously allocated
                                            // by CAM_AUTOCAM_USE_BUF
                                            // EFFECT ON PORT
                                            
    CAM_AUTOCAM_DEQUEUE_BUF,                // [OUT] CAM_ImageBuffer **ppBuf
                                            // dequeue from filled buffer queue.
                                            // Note that this is a blocking call.
                                            // EFFECT ON PORT
                                            
    CAM_AUTOCAM_ENQUEUE_BUF,                // [IN] CAM_ImageBuffer *pBuf
                                            // enqueue to empty buffer queue.
                                            // EFFECT ON PORT

    CAM_AUTOCAM_SET_BURSTCOUNT,             // [IN] int *pCount
                                            // set the target length of buffer queue
                                            // EFFECT ON PORT 

    // Format and resolution control commands
    CAM_AUTOCAM_SET_PREVIEW_SIZE,           // [IN] CAM_Size *szPreviewSize
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_PREVIEW_FORMAT,         // [IN] CAM_ImageFormat *pFormat
                                            // EFFECT ON CAMERA

    CAM_ATUOCAM_SET_PREVIEW_RESINDEX,       // [IN] int *pResolutionIndex
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_PREVIEW_FLIPROTATE,     // [IN] CAM_FlipRotate *eFlipRotate
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_PREVIEW_AUTOCROP,       // [IN] CAM_Bool *pBool
                                            // EFFECT ON CAMERA
    
    CAM_AUTOCAM_SET_CAPTURE_FORMAT,         // [IN] CAM_ImageFormat *pFormat
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_CAPTURE_RESINDEX,       // [IN] int *pResolutionIndex
                                            // EFFECT ON CAMERA


    // AF control commands
    CAM_AUTOCAM_AF_START,                   // [NULL]
                                            // start auto-focus process
                                            // EFFECT ON CAMERA
                                            
    CAM_AUTOCAM_AF_CHECK,                   // [OUT] CAM_Int16u *pAFStatus
                                            // check auto-focus status
                                            // EFFECT ON CAMERA
                                            
    CAM_AUTOCAM_AF_CANCEL,                  // [NULL]
                                            // cancel auto-focus process
                                            // EFFECT ON CAMERA
                                            
        
    // Photographic control commands
    CAM_AUTOCAM_SET_PHOTOMODE,              // [IN] CAM_PhotoMode *pPhotoMode
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_ISO,                    // [IN] CAM_ISOMode *pISO
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_EVCOMPENSATION,         // [IN] CAM_Int32s *pEVCompensationQ16
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_SHUTTERSPEEDINDEX,      // [IN] int *pShutterSpeedIndex
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_FNUMINDEX,              // [IN] int *pFNumIndex
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_COLOREFFECT,            // [IN] CAM_ColorEffect *pColorEffect
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_WHITEBALANCEMODE,       // [IN] CAM_WhiteBalanceMode *pWhiteBalance
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_SATURATION,             // [IN] int *pSaturationIndex
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_CONTRAST,               // [IN] int *pContrastIndex
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_EXPOSUREMEASUREMODE,    // [IN] int *pExposureMeasureModeIndex  
                                            // EFFECT ON CAMERA

    CAM_AUTOCAM_SET_BANDFILTER,             // [IN] int *pBandFilterIndex
                                            // EFFECT ON CAMERA
  
    CAM_AUTOCAM_SET_NIGHTMODE,              // [IN] CAM_Bool *enable    
                                            // EFFECT ON CAMERA                     
} CAM_AutoCamCommand;


#ifdef __cplusplus
}
#endif

#endif  // _CAM_AUTOCAM_H_


