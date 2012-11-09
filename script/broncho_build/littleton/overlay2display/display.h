#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/types.h>


#define FORMAT_RGB              0x0
#define FORMAT_PACKED_444       0x1
#define FORMAT_PLANAR_444       0x2
#define FORMAT_PLANAR_422       0x3
#define FORMAT_PLANAR_420       0x4
#define MIN(x, y)   ((x < y) ? x : y)
#define SCREEN_WIDTH		240
#define SCREEN_HEIGHT		320

#define FB_VMODE_YUV420PLANAR	FORMAT_PLANAR_420	//workaround for android_surface_output.cpp 

#define  _ALIGN16(adr)   ((((unsigned char)(adr))+15)&(~15))


typedef struct {
    int	fd_lcd;
    int	bpp;
    int	format;
    int	step;
    int	height; 
    int screen_width;
    int screen_height;
    int	screen_pos_x;
    int	screen_pos_y;
    int	yoff; 
    int	ylen;
    int	cboff;
    int	cblen;
    int	croff;
    int	crlen;
    int	fix_smem_len;
    unsigned char *map;
    unsigned char *overlay[3];
    int configDone;
} display_cfg;

int get_base_fb_width();
int get_base_fb_height();
int display_open(display_cfg **ppDispCfg);
int display_config(display_cfg *pDispCfg);
int display_close(display_cfg **ppDispCfg);
int copy_to_overlay(display_cfg *pDispCfg, unsigned char *pSrc[3]);
int get_overlay_yuv(display_cfg *pDispCfg, unsigned char **y, unsigned char**u, unsigned char **v);
int ReadYUVFrame(FILE* infile, unsigned int picWidth, unsigned int picHeight, unsigned char* pPicture[3]);

