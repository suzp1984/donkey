#include "display.h"
#include <utils/Log.h>

#define BASE_FB_DEVICE "/dev/graphics/fb0"
#define OVERLAY2_FB_DEVICE "dev/graphics/fb2"
int get_base_fb_width() {
    int fd = 0;
    int ret = 0;
    struct fb_var_screeninfo var_base;

    fd = open(BASE_FB_DEVICE, O_RDWR);
    if( fd < 0 )
    	return -1;
    ret = ioctl(fd, FBIOGET_VSCREENINFO, &var_base);
    close(fd);
    if( ret < 0 )
	return -1;
    else
	return var_base.xres;
}

int get_base_fb_height() {
    int fd = 0;
    int ret = 0;
    struct fb_var_screeninfo var_base;

    fd = open(BASE_FB_DEVICE, O_RDWR);
    if( fd < 0 )
    	return -1;
    ret = ioctl(fd, FBIOGET_VSCREENINFO, &var_base);
    close(fd);
    if( ret < 0 )
	return -1;
    else
	return var_base.yres;
}

int display_open(display_cfg **ppDispCfg) {
    int fd;
    display_cfg *pDispCfg=NULL;
    fd = open(OVERLAY2_FB_DEVICE, O_RDWR);
    if (fd < 0)
    {
        *ppDispCfg = NULL;
        return -1;
    }
    if((pDispCfg = (display_cfg*)malloc(sizeof(display_cfg)))) 
    {
        memset(pDispCfg, 0, sizeof(display_cfg));
        pDispCfg->fd_lcd = fd;
        pDispCfg->map = NULL;
    }
    else
    {
        *ppDispCfg = NULL;
        return -1;
    }
    *ppDispCfg = pDispCfg;
    return 0;
}

int overlay2_mmap(display_cfg *pDispCfg) 
{
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    int err;

    if (pDispCfg == NULL) return -1;

    memset(&fix,0,sizeof(fix));
    memset(&var,0,sizeof(var));
   
    var.xres = pDispCfg->screen_width;
    var.yres = pDispCfg->screen_height;
    var.bits_per_pixel = pDispCfg->bpp;
    /* nonstd for FORMAT/XPOS/YPOS */
    var.nonstd = (pDispCfg->format << 20) | (pDispCfg->screen_pos_y << 10) | pDispCfg->screen_pos_x;

    /* set "var" screeninfo */
    err = ioctl(pDispCfg->fd_lcd, FBIOPUT_VSCREENINFO, &var);
    if (err) {
        close(pDispCfg->fd_lcd);
        return -3;
    }
    /* get updated "var" screeninfo */
    err = ioctl(pDispCfg->fd_lcd, FBIOGET_VSCREENINFO, &var);
    if (err) {
        close(pDispCfg->fd_lcd);
        return -4;
    }
    /* get fix screeninfo */
    // comments of hurri, i am confused by the fixed screen, because it seems it is not fixed, the fix.smem_len will
    // be changed based ont var.xres and var.yres. so we must get the fixed screen info after we have configured
    // variable screen because the fix.smem_len is critical.
    err = ioctl(pDispCfg->fd_lcd, FBIOGET_FSCREENINFO, &fix);
    if (err) {
        return -2;
    }
   
    LOGV("Overlay2 configuration: %d*%d@%d-bit, smem_len=%d\n", var.xres, var.yres, var.bits_per_pixel, fix.smem_len);

    pDispCfg->map = (unsigned char*)mmap(0, fix.smem_len, PROT_READ | PROT_WRITE,MAP_SHARED, pDispCfg->fd_lcd, 0);

    if (MAP_FAILED == pDispCfg->map) return -5;

    pDispCfg->yoff   = var.red.offset;
    pDispCfg->ylen   = var.red.length;
    pDispCfg->cboff  = var.green.offset;
    pDispCfg->cblen  = var.green.length;
    pDispCfg->croff  = var.blue.offset;
    pDispCfg->crlen  = var.blue.length;

    pDispCfg->fix_smem_len = fix.smem_len;

    return 0;
}

int overlay2_unmap(display_cfg *pDispCfg) 
{
    int ret;
    ret = munmap(pDispCfg->map, pDispCfg->fix_smem_len);
    return(ret);
}

int display_config(display_cfg *pDispCfg) {
    int fd, ret;
    struct fb_var_screeninfo var_base;
    if(pDispCfg == NULL)
    {
        return -1;
    }
	
    /* Get var info base fb */
    fd  = open(BASE_FB_DEVICE, O_RDWR);
    ret = ioctl(fd, FBIOGET_VSCREENINFO, &var_base);
    close(fd);
    LOGV("base fb configuration: %d*%d@%d-bit, nonstd=%x\n", var_base.xres, var_base.yres, var_base.bits_per_pixel, var_base.nonstd);
	
    /* fb2's resolution can not exceeds base fb */
    if (pDispCfg->screen_width > var_base.xres) pDispCfg->screen_width = var_base.xres;
    if (pDispCfg->screen_height > var_base.yres) pDispCfg->screen_height = var_base.yres;	

    if (pDispCfg->map != NULL)
    {
        overlay2_unmap(pDispCfg);
    }

    ret = overlay2_mmap(pDispCfg);

    if (ret != 0)
    {
        return ret;
    }

    pDispCfg->overlay[0] = pDispCfg->map + pDispCfg->yoff;
    pDispCfg->overlay[1] = pDispCfg->map + pDispCfg->cboff;
    pDispCfg->overlay[2] = pDispCfg->map + pDispCfg->croff;

    return 0;
}

int display_close(display_cfg **ppDispCfg) {
    display_cfg *pDispCfg = *ppDispCfg;
    if( pDispCfg == NULL )
    {
        return -1;
    }
    if (pDispCfg->fd_lcd > 0) {
        overlay2_unmap(pDispCfg);
        close(pDispCfg->fd_lcd);
    }
    free(pDispCfg);
    *ppDispCfg = NULL;
    return 0;
}

int copy_to_overlay(display_cfg *pDispCfg, unsigned char *pSrc[3]) {
    int width, height;
    int screen_width, screen_height, format;
    int i;
    int heightY, heightCb, heightCr;
    int dstHeightY, dstHeightCb, dstHeightCr;
    int stepY, stepCb, stepCr;
    int dststepY, dststepCb, dststepCr;
    unsigned char* srcPtr, *dstPtr;
    int miniStep, miniHeight;

    if (pDispCfg == NULL) return -1;

    width = pDispCfg->step;
    height = pDispCfg->height;
    screen_width = pDispCfg->screen_width;
    screen_height = pDispCfg->screen_height;
    format = pDispCfg->format;

    if (format == FORMAT_PLANAR_422) {
        heightY      = height;
        heightCb     = height;
        heightCr     = height;
        dstHeightY   = screen_height;
        dstHeightCb  = screen_height;
        dstHeightCr  = screen_height;
        stepY        = width;
        stepCb       = width / 2;
        stepCr       = width / 2;
        dststepY     = screen_width;
        dststepCb    = screen_width / 2;
        dststepCr    = screen_width / 2;
    } else if (format == FORMAT_PLANAR_420) {
        heightY      = height;
        heightCb     = height / 2;
        heightCr     = height / 2;
        dstHeightY   = screen_height;
        dstHeightCb  = screen_height / 2;
        dstHeightCr  = screen_height / 2;
        stepY        = width;
        stepCb       = width / 2;
        stepCr       = width / 2;
        dststepY     = screen_width;
        dststepCb    = screen_width / 2;
        dststepCr    = screen_width / 2;
    } else
        return -1;
    // copy Y component
    srcPtr = pSrc[0]; 
    dstPtr = pDispCfg->overlay[0];
    miniStep = MIN(stepY, dststepY);
    miniHeight = MIN(heightY, dstHeightY);
    for (i = 0; i < miniHeight; i++) {
        memcpy(dstPtr, srcPtr, miniStep);
        srcPtr += stepY;
        dstPtr += dststepY;
    }

    // copy Cb component
    srcPtr = pSrc[1];
    dstPtr = pDispCfg->overlay[1];
    miniStep = MIN(stepCb, dststepCb);
    miniHeight = MIN(heightCb, dstHeightCb);
    for (i = 0; i < miniHeight; i++) {
        memcpy(dstPtr, srcPtr, miniStep);
        srcPtr += stepCb;
        dstPtr += dststepCb;
    }

    // copy Cr component
    srcPtr = pSrc[2];
    dstPtr = pDispCfg->overlay[2];
    miniStep = MIN(stepCr, dststepCr);
    miniHeight = MIN(heightCr, dstHeightCr);
    for (i = 0; i < miniHeight; i++) {
        memcpy(dstPtr, srcPtr, miniStep);
        srcPtr += stepCr;
        dstPtr += dststepCr;
    }
    return 0;
}

// ReadYUVFrame only used for test with raw yuv data file
#if 0
int ReadYUVFrame(FILE* infile, unsigned int picWidth, unsigned int picHeight, unsigned char* pPicture[3])
{
    unsigned char* tempPtr1;
    int j;
    tempPtr1 = pPicture[0];
    /* Reload YUV plane */
    for(j = 0; j < picHeight; j++) {
        if ( picWidth != fread(tempPtr1,1,picWidth,infile) ) {
            return -1;
        }
        tempPtr1 += picWidth;
    }

    tempPtr1 = pPicture[1];
    for(j = 0; j < picHeight/2; j++) {
        if ( (picWidth/2) != fread(tempPtr1,1,picWidth/2, infile) ) {
            return -1;
        }
        tempPtr1 += (picWidth/2);
    }
    tempPtr1 = pPicture[2];

    for(j = 0; j < picHeight/2; j++) {
        if ( (picWidth/2) != fread(tempPtr1,1,picWidth/2, infile) ) {
            return -1;
        }
        tempPtr1 += (picWidth/2);
    }
    return 0;
}
#endif

// get the overlayer2 yuv pointer, it must be called after display configure called
int get_overlay_yuv(display_cfg *pDispCfg, unsigned char **y, unsigned char**u, unsigned char **v)
{
    int ret = -1;
    if( (pDispCfg != NULL) && (pDispCfg->fd_lcd > 0) )
    {
        ret = 0;
        *y = pDispCfg->overlay[0];
        *u = pDispCfg->overlay[1];
        *v = pDispCfg->overlay[2];
    }
    return ret;
}

