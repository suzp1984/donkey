/*******************************************************************************
//(C) Copyright [2009] Marvell International Ltd.
//All Rights Reserved
*******************************************************************************/

#include "sensor_stub.h"

/* TODO: sensor specific resolution table */
CAM_SensorResolution _ResTable[] = {
    {0,  320,  240,  1024, 768,  209715/*3.2x*/},
    {1,  640,  480,  1024, 768,  104856/*1.6x*/},
    {2,  1024, 768,  1024, 768,  65536/*1x*/},
    {3,  2048, 1536, 2048, 1536, 65536/*1x*/}
};

const int _sizeResTable = _ARRAY_SIZE(_ResTable);

/*
 * utility functions
 */
// get sensor register value
// Parameters
//      reg         - register's address
//      lbit, hbit  - start and end bit in src 8-bit register to get, 0<=lbit<=hbit<=7
//      val_lbit    - start bit in dest 16-bit val storing the bits gotten from src register
//      val         - pointer to the value to store the bits got from the register
int __GET_SENSOR_REG(int iDeviceID, CAM_Int16u reg, int lbit, int hbit, int val_lbit, CAM_Int16u *val)
{
#ifdef QCI
    CAM_Int16u mask;
    int nbits = hbit - lbit + 1;
    struct reg_set_s s = {reg, 0};

    if (0 != ioctl(iDeviceID, WCAM_VIDIOSENSORR, &s))
    {
        TRACE("%s ERROR: %s\n", __FUNCTION__, strerror(errno));
        return 1;
    }

    mask = ( ((1 << nbits) - 1) << val_lbit );
    (*val) &= ~mask;
    (*val) |= ( (s.val2 >> lbit) << val_lbit ) & mask;

    return 0;
#else
    CAM_Int16u mask;
    int nbits = hbit - lbit + 1;
    struct v4l2_register s;

    s.reg = reg;
    s.val = 0;

    if (0 != ioctl(iDeviceID, VIDIOC_DBG_G_REGISTER, &s))
    {
        TRACE("%s ERROR: %s\n", __FUNCTION__, strerror(errno));
        return 1;
    }

    mask = ( ((1 << nbits) - 1) << val_lbit );
    (*val) &= ~mask;
    (*val) |= ( (s.val >> lbit) << val_lbit ) & mask;

    return 0;
#endif
}


// set sensor register value
// Parameters
//      reg         - register's address
//      lbit, hbit  - start and end bit in dest 8-bit register to set, 0<=lbit<=hbit<=7
//      val_lbit    - the start bit in src 16-bit val to be set to register
//      val         - the value that contains the bits that need to set to the register
int __SET_SENSOR_REG(int iDeviceID, CAM_Int16u reg, int lbit, int hbit, int val_lbit, CAM_Int16u val)
{
#ifdef QCI
    CAM_Int16u mask, val0;
    int nbits = hbit - lbit + 1;
    struct reg_set_s s = {reg, 0};
    int ret = 0;

//  TRACE("0x%x[%d:%d] <-- 0x%x[%d:%d]\n", reg, lbit, hbit, val, val_lbit, val_lbit+hbit-lbit);
    if (0 != ioctl(iDeviceID, WCAM_VIDIOSENSORR, &s))
    {
        return 1;
    }
    TRACE("%s: 0x%x[0x%x]", __FUNCTION__, s.val1, s.val2);

    mask = ( ((1 << nbits) - 1) << lbit );
    val0 = s.val2;
    s.val2 &= ~mask;
    s.val2 |= ( (val >> val_lbit) << lbit ) & mask;

    if (val0 != s.val2)
    {
        ret = ioctl(iDeviceID, WCAM_VIDIOSENSORW, &s);

#ifdef ASSERT
{
    CAM_Int16u val1 = val;
    __GET_SENSOR_REG(iDeviceID, reg, lbit, hbit, val_lbit, &val1);
    val &= 0xff;
    val1 &= 0xff;
    if (val1 != val)
    {
        ASSERT(val1 == val);
        TRACE("_SET_SENSOR_REG failed: 0x%x[%d:%d] <-- 0x%x[%d:%d]\n", reg, lbit, hbit, val, val_lbit, val_lbit+hbit-lbit);
        TRACE("target: 0x%x, actual: 0x%x\n", val, val1);
        return 1;
    }
}
#endif
    }

#ifdef CAM_LOG_VERBOSE
{
    s.val1 = reg;
    s.val2 = 0;
    ioctl(iDeviceID, WCAM_VIDIOSENSORR, &s);
    TRACE("--> 0x%x[0x%x]\n", s.val1, s.val2);
}
#endif

#else
    CAM_Int16u mask, val0;
    int nbits = hbit - lbit + 1;
    struct v4l2_register s;
    int ret = 0;

    s.reg = reg;
    s.val = 0;
//  TRACE("0x%x[%d:%d] <-- 0x%x[%d:%d]\n", reg, lbit, hbit, val, val_lbit, val_lbit+hbit-lbit);
    if (0 != ioctl(iDeviceID, VIDIOC_DBG_G_REGISTER, &s))
    {
        return 1;
    }
    TRACE("%s: 0x%x[0x%x]", __FUNCTION__, (__u16)s.reg, (__u16)s.val);

    mask = ( ((1 << nbits) - 1) << lbit );
    val0 = s.val;
    s.val &= ~mask;
    s.val |= ( (val >> val_lbit) << lbit ) & mask;

    if (val0 != s.val)
    {
        ret = ioctl(iDeviceID, VIDIOC_DBG_S_REGISTER, &s);

#ifdef ASSERT
{
    CAM_Int16u val1 = val;
    __GET_SENSOR_REG(iDeviceID, reg, lbit, hbit, val_lbit, &val1);
    val &= 0xff;
    val1 &= 0xff;
    if (val1 != val)
    {
        ASSERT(val1 == val);
        TRACE("_SET_SENSOR_REG failed: 0x%x[%d:%d] <-- 0x%x[%d:%d]\n", reg, lbit, hbit, val, val_lbit, val_lbit+hbit-lbit);
        TRACE("target: 0x%x, actual: 0x%x\n", val, val1);
        return 1;
    }
}
#endif
    }

#ifdef CAM_LOG_VERBOSE
{
    s.reg = reg;
    s.val = 0;
    ioctl(iDeviceID, VIDIOC_DBG_G_REGISTER, &s);
    TRACE("--> 0x%x[0x%x]\n", (__u16)s.reg, (__u16)s.val);
}
#endif

#endif
    return ret;
}

 
CAM_Error SetRegArray(int iDeviceID, const CAM_Int16u *p, int size)
{
    int i;
    CAM_Error error = CAM_ERROR_NONE;

    ASSERT(p);

    for (i=0; i<size; i+=5){
        __SET_SENSOR_REG(iDeviceID, p[i], p[i+1], p[i+2], p[i+3], p[i+4]);
    } 

    return error;   
}


#define DIV_ROUND(a, b) ((a) + ((b)>>1)) / (b)
#define DIV_CEIL(a, b) ((a) + (b) - 1) / (b)

CAM_Error _calc_still_exposure (CAM_Int16u iDeviceID, 
                                CAM_AECState *pAECState)
{
    /* TODO: add sensor specific implementation here */    
    return CAM_ERROR_NONE;
}

CAM_Error _set_still_exposure(CAM_Int16u iDeviceID, 
                              const CAM_AECState *pAECState)
{
    /* TODO: add sensor specific implementation here */
    return CAM_ERROR_NONE;  
}

