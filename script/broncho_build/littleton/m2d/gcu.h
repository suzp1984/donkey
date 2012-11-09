/******************************************************************************
 *(C) Copyright 2009 Marvell International Ltd.  
 * All Rights Reserved 
 *
******************************************************************************/

#ifndef __GCU_H__
#define __GCU_H__

#include "m2d_lib.h"

#ifdef BMM_ENABLE

#include "bmm_lib.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#define FORMAT_RGB                     0x0
#define FORMAT_PACKED_444       0x1
#define FORMAT_PLANAR_444       0x2
#define FORMAT_PLANAR_422       0x3
#define FORMAT_PLANAR_420       0x4

/* Used for YCrCb data, it is stored in three areas */
struct v_frame {
    	int height;
    	int width;
    	void *  start[3];
    	size_t  length[3];
};


/* Used for gcu device */
struct gcu_info {

	struct m2d_context *context; 		

	struct m2d_buff *src_buf[3];
	struct m2d_buff *dst_buf[3];
	struct m2d_buff *mid_buf[3];			/*use for temp buffer*/
	struct m2d_buff *rot_buf[3];			/*use for rotate buffer*/

	/* used for bmm malloc and free */
	void * mid_buf_phy[3];
	void * rot_buf_phy[3];


	/* we support 497*2 pixels max width, so here two region  */
	struct m2d_op_region opr[3][2];
	struct m2d_op_region mid_opr[3][2];
	struct m2d_op_region rot_opr[3][2];
	
	/*src info*/
	int src_height[3];
	int src_width[3];

	/*dst info*/ 
	int dst_height[3];
	int dst_width[3];

	/*mid info*/ 
	int mid_height[3];
	int mid_width[3];

	/* format for YUV */
	int format;

	/* resize time */
	int resize_times;
	int resize_times_2;
	int rotate_times;

	int resizetype;	

	int rotate_degree;

	int use_srcbuf;
	int gcu_enable;


	/* MVED work around */
	int use_workaround;
	int pic_size;

};



int gcu_open(struct gcu_info *gcu, struct v_frame *src);

int gcu_close(struct gcu_info *gcu);

int gcu_parse(struct gcu_info *gcu, struct v_frame *src, struct v_frame *dst);

int gcu_process(struct gcu_info *gcu, struct v_frame *src, struct v_frame *dst);


void gcu_sync(struct gcu_info *gcu);

void gcu_set_rot_degree(struct gcu_info *gcu, int deg);
int gcu_get_rot_degree(struct gcu_info *gcu);

void gcu_set_use_srcbuf(struct gcu_info *gcu, int val);
int gcu_get_use_srcbuf(struct gcu_info *gcu);
int gcu_new_dst_buffer(struct gcu_info *gcu, struct v_frame *dst);
int gcu_free_rot_buffer(struct gcu_info *gcu);

#ifdef __cplusplus
	}
#endif
#endif
