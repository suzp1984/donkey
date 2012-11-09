/*******************************************************************************
//(C) Copyright [2009] Marvell International Ltd.
//All Rights Reserved
*******************************************************************************/

/*
 * This is a sample implementation of L2 layer, where all
 * hardware related stuff goes. 
 */

#define _ERROR_DOMAIN_  "L2"


/*
 * Camera Engine Header Files
 */
#include "CamGen.h"
#include "CamSubComp.h"


/*
 * The entry for L1 access. Do NOT change.
 */
static CAM_Error subcomp_open(void **ppDeviceData);
static CAM_Error subcomp_close(void **ppDeviceData);
static CAM_Error subcomp_command(void *pDeviceData, CAM_Command cmd, int param1, CAM_Parameter param2);

CAM_DriverEntry entry_subcomp = {
    "subcomponents",
    subcomp_open,
    subcomp_close,
    subcomp_command,
};


/*
 * TODO: hardware specific header files
 */
#include "sensor_stub.h"


/*
 * TODO: internal structures of subcomp
 */
/* Sensor definitions */
typedef struct {
    int                     iResolutionTableSize;
    CAM_SensorResolution    *pResolutionTable; 

    // The table of all supported analog exposure gain settings, table data
    // stored in INCREMENTAL order, gain value format is of Q7.8
    int                 iAnalogGainTableSize;       // Entry number of the table
    CAM_Int16s          *pAnalogGainTableQ8;        // Analog gain table

    // The table of all supported digital exposure gain settings, table data
    // stored in INCREMENTAL order, gain value format is of Q7.8
    int                 iDigitalGainTableSize;      // Entry number of the table
    CAM_Int16s          *pDigitalGainTableQ8;       // Digital gain table
} CAM_SensorProperty;

/* ISP definitions */
typedef struct _CAM_ISPProperty {
    int                 iAWBTableSize;
    CAM_RegTableEntry   *pAWBTable;
    int                 iColorEffectTableSize;
    CAM_RegTableEntry   *pColorEffectTable;
    int                 iISOTableSize;
    CAM_RegTableEntry   *pISOTable;
    int                 iSaturationTableSize;
    CAM_RegTableEntry   *pSaturationTable;
    int                 iContrastTableSize;
    CAM_RegTableEntry   *pContrastTable;
    int                 iExposureMeasureTableSize;
    CAM_RegTableEntry   *pExposureMeasureTable;
    int                 iBandfilterTableSize;
    CAM_RegTableEntry   *pBandfilterTable;
    int                 iEVTableSize;
    CAM_RegTableEntry   *pEVTable;
    CAM_AECState     AECState;     
} CAM_ISPProperty;


typedef struct {
    CAM_SensorProperty  SensorProperty;
    CAM_ISPProperty     ISPProperty;
} CAM_SubCompState;


/*
 * TODO: implement your own entry functions 
 */
static CAM_Error subcomp_open(void **ppDeviceData)
{
    CAM_SubCompState *pState = NULL;
    
    pState = (CAM_SubCompState *)malloc(sizeof(CAM_SubCompState));
    if (pState == NULL) {
        ERROR("failed to malloc the subcomp state structure\n");
        return CAM_ERROR_OUTOFMEMORY;
    }
    memset(pState, 0, sizeof(CAM_SubCompState));
    (*ppDeviceData) = pState;

    pState->SensorProperty.iResolutionTableSize     = _sizeResTable;
    pState->SensorProperty.pResolutionTable         = _ResTable;
    pState->ISPProperty.iAWBTableSize               = 0;
    pState->ISPProperty.pAWBTable                   = 0;
    pState->ISPProperty.iColorEffectTableSize       = 0;
    pState->ISPProperty.pColorEffectTable           = 0;
    pState->ISPProperty.iISOTableSize               = 0;
    pState->ISPProperty.pISOTable                   = 0;
    pState->ISPProperty.iSaturationTableSize        = 0;
    pState->ISPProperty.pSaturationTable            = 0;
    pState->ISPProperty.iContrastTableSize          = 0;
    pState->ISPProperty.pContrastTable              = 0;
    pState->ISPProperty.iExposureMeasureTableSize   = 0;
    pState->ISPProperty.pExposureMeasureTable       = 0;
    pState->ISPProperty.iBandfilterTableSize        = 0;
    pState->ISPProperty.pBandfilterTable            = 0;
    pState->ISPProperty.iEVTableSize                = 0;
    pState->ISPProperty.pEVTable                    = 0;

    return CAM_ERROR_NONE;
}


static CAM_Error subcomp_close(void **ppDeviceData)
{
    CAM_SubCompState *pState = (CAM_SubCompState*)*ppDeviceData;
    CAM_Error error = CAM_ERROR_NONE;

    if ( NULL == pState ) {
        ERROR("pState should not be NULL\n");
        return CAM_ERROR_BADARGUMENT;
    };

    free(pState);
    *ppDeviceData = NULL;

    return error;
}


static CAM_Error subcomp_command(void *pDeviceData,
                                 CAM_Command cmd,
                                 int param1,
                                 CAM_Parameter param2)
{
    int iDeviceID = param1; /* camera device ID */
    CAM_SubCompState *pState = (CAM_SubCompState *)pDeviceData;
    CAM_Error error = CAM_ERROR_NONE;

    if (NULL == pState) {
        ERROR("pState shouldn't be NULL\n");
        return CAM_ERROR_BADARGUMENT;
    }
  
    switch (cmd) {
    /*
     * TODO: implement MANDATORY commands 
     */	
    case CAM_SENSOR_GET_RESOLUTION:
    { 
        CAM_SensorResolution *pSensorRes = (CAM_SensorResolution *)param2;
        CAM_SensorResolution *pResTable = pState->SensorProperty.pResolutionTable;
        int iResIndex = pSensorRes->index;

        if (iResIndex < 0 || 
            iResIndex >= pState->SensorProperty.iResolutionTableSize) {
            error = CAM_ERROR_BADARGUMENT;
            ERROR("resolution index (%d) isn't in [%d, %d]\n",
                  iResIndex, 0, pState->SensorProperty.iResolutionTableSize - 1);
        }
        else {
            pSensorRes->iWidth        = pResTable[iResIndex].iWidth;
            pSensorRes->iHeight       = pResTable[iResIndex].iHeight;
            pSensorRes->iRawWidth     = pResTable[iResIndex].iRawWidth;
            pSensorRes->iRawHeight    = pResTable[iResIndex].iRawHeight;
            pSensorRes->iMaxZoomInQ16 = pResTable[iResIndex].iMaxZoomInQ16;
            error = CAM_ERROR_NONE;
        }
         
    } break;

                                        
    case CAM_ISP_ENABLE_AE:
#if 0
        if (CAM_TRUE ==  *(CAM_Bool *)param2) {
            _SET_SENSOR_REG(iDeviceID, OV3640_REG_AECENABLE, 1);
            _SET_SENSOR_REG(iDeviceID, OV3640_REG_AGCENABLE, 1);
        }
        else {
            _SET_SENSOR_REG(iDeviceID, OV3640_REG_AECENABLE, 0);
            _SET_SENSOR_REG(iDeviceID, OV3640_REG_AGCENABLE, 0);
        }      
#endif
        error = CAM_ERROR_NONE;
        break;
    

    case CAM_ISP_ENABLE_AWB:
#if 0
        if (CAM_TRUE ==  *(CAM_Bool *)param2) {
            _SET_SENSOR_REG(iDeviceID, OV3640_REG_MANUALWB, 0);
        }
        else {
            _SET_SENSOR_REG(iDeviceID, OV3640_REG_MANUALWB, 1);
        }      
#endif
        error = CAM_ERROR_NONE;
        break;

        
    case CAM_ISP_START_AF:
    {
#if 0
        CAM_Int16u AFState = 0x0;
    
        _GET_SENSOR_REG(iDeviceID, OV3640_REG_FWSTATE, &AFState);
        _SET_SENSOR_REG(iDeviceID, OV3640_REG_FWCMD, 0x01);
        
        if (OV3640_FWSTATE_INF == AFState) {
            _SET_SENSOR_REG(iDeviceID, OV3640_REG_FWCMD, 0x03);
            error = CAM_ERROR_NONE;
        }
        else {
            ERROR("Can't start AF: status (0x%x)\n", AFState);
            error = CAM_ERROR_OPFAILED;
        }
#endif
        error = CAM_ERROR_NONE;
    } break;    


    case CAM_ISP_CHECK_AF:
#if 0
        _GET_SENSOR_REG(iDeviceID, OV3640_REG_FWSTATE, param2);   
#endif
        break;


    case CAM_ISP_CANCEL_AF: 
    {
#if 0
        CAM_Int16u AFState = 0x0;

        _GET_SENSOR_REG(iDeviceID, OV3640_REG_FWSTATE, &AFState);
        if (OV3640_FWSTATE_SUCCESS_S == AFState){
            _SET_SENSOR_REG(iDeviceID, OV3640_REG_FWCMD, 0x08);
            error = CAM_ERROR_NONE;
        }
        else {
            ERROR("Can't cancel AF: status (0x%x)\n", AFState);    
            error = CAM_ERROR_OPFAILED;
        }
#endif
        error = CAM_ERROR_NONE;
    } break;                


    case CAM_ISP_SAVE_3A:
        error = _calc_still_exposure(iDeviceID, &pState->ISPProperty.AECState);
        break;
        

    case CAM_ISP_RESTORE_3A:
        error = _set_still_exposure(iDeviceID, &pState->ISPProperty.AECState);
        break;
    
    /*
     * TODO: implement OPTIONAL commands,
     * or return CAM_ERROR_NOTIMPLEMENTED for Unsupported commands 
     */
    default:
        ERROR("Unsupported SubComp Command\n");
        error = CAM_ERROR_NOTIMPLEMENTED;
        break;
    }

    return error;
}
