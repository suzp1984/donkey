/******************************************************************************
 *(C) Copyright 2009 Marvell International Ltd.  
 * All Rights Reserved 
 *
******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "gcu.h"

/* 2d one time only solve 497 pixel, if bigger than this, more time is needed */
#define GCU_LIMIT 497

#define MOVE_ONLY 1
#define DECIMATE_ONLY 2
#define STRETCH_ONLY 3
#define DECIMATE_STRETCH 4

#define ROTATE_ONLY 5
#define ROTATE_STRETCH 6
#define DECIMATE_ROTATE 7
#define ROTATE_DECIMATE_STRETCH 8

//#define GCU_WORKAROUND
int gcu_free_rot_buffer(struct gcu_info *gcu)
{
	int i = 0;

	for (i=0; i<3; i++){		
		
		if(gcu->rot_buf[i] != NULL){
			
#ifdef BMM_ENABLE
			m2d_destroy_buffer(gcu->context, gcu->rot_buf[i]);
			if(gcu->rot_buf_phy[i]){
				bmm_free(gcu->rot_buf_phy[i]);
				gcu->rot_buf_phy[i] = NULL;
			}
#else				
			m2d_free_buffer(gcu->context, gcu->rot_buf[i]);
#endif
			gcu->rot_buf[i] = NULL;
		}			
	}

	return 0;
}

/* New a rotate buffer, be careful that width and height are switched in function.
     If rotating 180, width and height should be switched first to get original value. */
static int gcu_new_rot_buffer(struct gcu_info *gcu, int* width, int* height)
{
	int i = 0;
	int size = 0;	
		
	for(i=0; i<3; i++){
				
		if(NULL == gcu->rot_buf[i]){
			/*rotate buffer need replace width and length*/

#ifdef BMM_ENABLE
			//size = gcu->src_height[i] * gcu->src_width[i];
			size = height[i] * width[i];
			if(!(gcu->rot_buf_phy[i] = bmm_malloc(size, BMM_ATTR_DEFAULT))){
				goto err;
			}
			if(!(gcu->rot_buf[i] = m2d_create_buffer(gcu->context, height[i], width[i], GCU_PXLFMT_INDEXED_8, gcu->rot_buf_phy[i]))){
				//if(!(gcu->rot_buf[i] = m2d_create_buffer(gcu->context, gcu->src_height[i], gcu->src_width[i], GCU_PXLFMT_INDEXED_8, gcu->rot_buf_phy[i]))){
				goto err;
			}
			
#else				

			if(!(gcu->rot_buf[i] = m2d_alloc_buffer(gcu->context, height[i], width[i], GCU_PXLFMT_INDEXED_8))){
				goto err;
			}
		
#endif
		}
	}

	return 0;

err:
	puts("create rot_buf failed");
	gcu_free_rot_buffer(gcu);
	
	return -1;
}

static int gcu_free_mid_buffer(struct gcu_info *gcu)
{
	int i = 0;
	

	for (i=0; i<3; i++){
		
		if(gcu->mid_buf[i] != NULL){

#ifdef BMM_ENABLE
			m2d_destroy_buffer(gcu->context, gcu->mid_buf[i]);
			if(gcu->mid_buf_phy[i]){
				bmm_free(gcu->mid_buf_phy[i]);
				gcu->mid_buf_phy[i] = NULL;
			}
#else				
			m2d_free_buffer(gcu->context, gcu->mid_buf[i]);
#endif


			gcu->mid_buf[i] = NULL;
		}			
	}

	return 0;
}


static int gcu_new_mid_buffer(struct gcu_info *gcu)
{
	int i = 0;
	int size = 0;

	gcu_free_mid_buffer(gcu);

	for (i=0; i<3; i++){		
		

#ifdef BMM_ENABLE
			size = gcu->mid_width[i] * gcu->mid_height[i];
			if(!(gcu->mid_buf_phy[i] = bmm_malloc(size, BMM_ATTR_DEFAULT))){
				goto err;
			}
			if(!(gcu->mid_buf[i] = m2d_create_buffer(gcu->context, gcu->mid_width[i], gcu->mid_height[i], GCU_PXLFMT_INDEXED_8, gcu->mid_buf_phy[i]))){
				goto err;
			}
			
#else				

			if(!(gcu->mid_buf[i] = m2d_alloc_buffer(gcu->context, gcu->mid_width[i], gcu->mid_height[i], GCU_PXLFMT_INDEXED_8))){
				goto err;
			}
#endif		
	}

	return 0;
	
err:
	puts("create mid_buf failed");
	gcu_free_mid_buffer(gcu);
	return -1;
}

static int gcu_free_src_buffer(struct gcu_info *gcu)
{
	int i = 0;

	for (i=0; i<3; i++){
		
		if(gcu->src_buf[i] != NULL){
			if(gcu->use_srcbuf){
				m2d_destroy_buffer(gcu->context, gcu->src_buf[i]);
			}else{
				m2d_free_buffer(gcu->context, gcu->src_buf[i]);
			}
			gcu->src_buf[i] = NULL;
		}			
	}

	return 0;
}


static int gcu_new_src_buffer(struct gcu_info *gcu, struct v_frame *src)
{
	int i = 0;
	
	for (i=0; i<3; i++){
		
		if(gcu->use_srcbuf){
			/*this is hardware input, but for software input, we only use memcpy */
			if(gcu->src_buf[i] != NULL){
				//m2d_free_buffer(gcu->context, gcu->src_buf[i]);
				m2d_destroy_buffer(gcu->context, gcu->src_buf[i]);
				gcu->src_buf[i] = NULL;
			}	

			gcu->src_buf[i] = m2d_create_buffer(gcu->context, gcu->src_width[i], gcu->src_height[i], GCU_PXLFMT_INDEXED_8, src->start[i]);	
			
		}else{
			/*software input buffer, not continuous */			
			memcpy(m2d_buff_addr(gcu->src_buf[i]), src->start[i], src->length[i]);
		}
	}

	return 0;
err:
	gcu_free_src_buffer(gcu);
	return -1;
}

static int gcu_free_dst_buffer(struct gcu_info *gcu)
{
	int i = 0;

	for (i=0; i<3; i++){		
		if(gcu->dst_buf[i] != NULL){
			m2d_destroy_buffer(gcu->context, gcu->dst_buf[i]);
			gcu->dst_buf[i] = NULL;
		}			
	}

	return 0;
}

int gcu_new_dst_buffer(struct gcu_info *gcu, struct v_frame *dst)
{
	int i = 0;
	/* create dst buffer */
	for (i=0; i<3; i++){
		if(gcu->dst_buf[i] != NULL){
			m2d_destroy_buffer(gcu->context, gcu->dst_buf[i]);
			gcu->dst_buf[i] = NULL;
		}	
		gcu->dst_buf[i] = m2d_create_buffer(gcu->context, gcu->dst_width[i], gcu->dst_height[i], GCU_PXLFMT_INDEXED_8, dst->start[i]);
		if(!gcu->dst_buf[i]){
			puts("create buffer failed");
			goto err;
		}
	}	

	return 0;

err:
	gcu_free_dst_buffer(gcu);
	return -1;
}





/*one function for resize and rotate, rotate src and dst should replace*/
int gcu_create_op(struct m2d_op_region opr[3][2], int* src_width, int* src_height, int* dst_width, int* dst_height, int rotate)
{
	int i = 0;

	if((src_width[0] <= GCU_LIMIT)&& (src_width[0] > 0)){
		/* one time is ok */
		for(i=0; i<3; i++){
			SET_OP_REGION_SRC0(&opr[i][0], 0, 0, 0, 0, src_width[i], src_height[i]);			
		}
		return 0;

	}else if((src_width[0] > GCU_LIMIT)&& (src_width[0] <= GCU_LIMIT*2)){
		/* need two time */

		for(i=0; i<3; i++){
			if(0 == rotate || 2 == rotate){
				SET_OP_REGION_SRC0(&opr[i][0], 0, 0, 0, 0, src_width[i]/2, src_height[i]);
				SET_OP_REGION_SRC0(&opr[i][1], src_width[i]/2, 0, dst_width[i]/2, 0, src_width[i]/2, src_height[i]);		
				
			}else if(1 == rotate){
				SET_OP_REGION_SRC0(&opr[i][0], 0, 0, 0, 0, src_width[i]/2, src_height[i]);
				SET_OP_REGION_SRC0(&opr[i][1], src_width[i]/2, 0, 0, dst_height[i]/2, src_width[i]/2, src_height[i]);		

			}else if(3==rotate){
				SET_OP_REGION_SRC0(&opr[i][0], 0, 0, 0, dst_height[i]/2, src_width[i]/2, src_height[i]);
				SET_OP_REGION_SRC0(&opr[i][1], src_width[i]/2, 0, 0, 0, src_width[i]/2, src_height[i]);	
			}
		}
		return 1;
	}else{
		/* do not support  */
		
		puts("src width do not support");
		return -1;
	}
	
}

int gcu_open(struct gcu_info *gcu, struct v_frame *src)
{
	/*first step*/
	/* mknod /dev/m2d c 253 0*/

	int i = 0;

	if((src->width <= 0)|| (src->width > GCU_LIMIT*2)){
		printf("frame size only support (0, 497x2]!\n");
		goto error;
	}
	
	gcu->context = m2d_create_context();
	if(gcu->context == NULL){
		printf("failed: context created\n");			
		goto error;
	}

	gcu->format = FORMAT_PLANAR_420;

	for (i=0; i<3; i++){
		gcu->dst_buf[i] = NULL;
		gcu->src_buf[i] = NULL;
		gcu->mid_buf[i] = NULL;		
		gcu->rot_buf[i] = NULL;	

		gcu->mid_buf_phy[i] = NULL;		
		gcu->rot_buf_phy[i] = NULL;	
	}

	gcu->resize_times = 0;
	gcu->resize_times_2 = 0;
	gcu->rotate_times = 0;

	/* parse src, which only need once */

	switch(gcu->format)
	{
		case FORMAT_PLANAR_422:
			
			gcu->src_height[0] = src->height;
			gcu->src_height[1] = src->height;	
			gcu->src_height[2] = src->height;
			gcu->src_width[0] = src->width;
			gcu->src_width[1] = src->width/2;	
			gcu->src_width[2] = src->width/2;			
			break;

		case FORMAT_PLANAR_420:
			
			gcu->src_height[0] = src->height;
			gcu->src_height[1] = src->height/2;	
			gcu->src_height[2] = src->height/2;
			gcu->src_width[0] = src->width;
			gcu->src_width[1] = src->width/2;	
			gcu->src_width[2] = src->width/2;
			break;

		case FORMAT_PLANAR_444:
			
			gcu->src_height[0] = src->height;
			gcu->src_height[1] = src->height;	
			gcu->src_height[2] = src->height;
			gcu->src_width[0] = src->width;
			gcu->src_width[1] = src->width;	
			gcu->src_width[2] = src->width;
			break;

		default:
			printf("format error!\n");
			goto error;
	
	}	
	
	if(!gcu->use_srcbuf){
		/* if inbuf is continuous, we can use inbuf directly, if not, we should alloc continuous buffer */

		/* create src buffer */
		for(i=0; i<3; i++){
			/* now we use memcpy, from inbuf to src_buff, which is physically continous */
			if(gcu->src_buf[i] != NULL){
				m2d_free_buffer(gcu->context, gcu->src_buf[i]);
				gcu->src_buf[i] = NULL;
			}			

			//gcu->src_buf[i] = m2d_alloc_buffer(gcu->context, gcu->src_height[i], gcu->src_width[i], GCU_PXLFMT_INDEXED_8);	
			gcu->src_buf[i] = m2d_alloc_buffer(gcu->context, gcu->src_width[i], gcu->src_height[i], GCU_PXLFMT_INDEXED_8);	
			if(!gcu->src_buf[i]){
				puts("gcu malloc src buffer failed");
				goto error;
			}
		}	
	}

	gcu->pic_size = 0;
	gcu->use_workaround = FALSE;	
	gcu->gcu_enable = TRUE;
	return 0;

error:
	
	if(gcu->context != NULL){			
		gcu_free_src_buffer(gcu);		
		m2d_free_context(gcu->context);
	}		
	return -1;
	
}


int gcu_parse(struct gcu_info *gcu, struct v_frame *src, struct v_frame *dst)
{	
	int i = 0;
	gcu->use_workaround = FALSE;	


	/* parse dst, which need reparse if screen modified */
	
	switch(gcu->format)
	{
		case FORMAT_PLANAR_422:
			
			gcu->dst_height[0] = dst->height;
			gcu->dst_height[1] = dst->height;	
			gcu->dst_height[2] = dst->height;
			gcu->dst_width[0] = dst->width;
			gcu->dst_width[1] = dst->width/2;	
			gcu->dst_width[2] = dst->width/2;
			
			break;

		case FORMAT_PLANAR_420:
			
			gcu->dst_height[0] = dst->height;
			gcu->dst_height[1] = dst->height/2;	
			gcu->dst_height[2] = dst->height/2;
			gcu->dst_width[0] = dst->width;
			gcu->dst_width[1] = dst->width/2;	
			gcu->dst_width[2] = dst->width/2;
			
			break;

		case FORMAT_PLANAR_444:
			
			gcu->dst_height[0] = dst->height;
			gcu->dst_height[1] = dst->height;	
			gcu->dst_height[2] = dst->height;
			gcu->dst_width[0] = dst->width;
			gcu->dst_width[1] = dst->width;	
			gcu->dst_width[2] = dst->width;

			break;

		default:
			printf("format error!\n");
			return -1;
	
	}	

	if(0 != gcu->rotate_degree && GCU_ROTATE_180 != gcu->rotate_degree){
	//if(0 != gcu->rotate_degree){
		/* need rotate */

		if((dst->height == src->width)&&(dst->width == src->height)){
			/* do not use buffer, just out rotate directly */
			gcu->resizetype = ROTATE_ONLY;

			gcu->rotate_times = gcu_create_op(gcu->opr, gcu->src_width, gcu->src_height, gcu->src_height, gcu->src_width, gcu->rotate_degree);
			
		}else if((dst->height >= src->width)&&(dst->width >= src->height)){
			/* stretch, should rotate first, then stretch, temp buffer needed by rotate */
			gcu->resizetype = ROTATE_STRETCH;
			
			if(gcu_new_rot_buffer(gcu, gcu->src_width, gcu->src_height)<0){
				puts("create rot_buf failed");
				goto error;
			}

			gcu->rotate_times = gcu_create_op(gcu->rot_opr, gcu->src_width, gcu->src_height, gcu->src_height, gcu->src_width, gcu->rotate_degree);				
			gcu->resize_times = gcu_create_op(gcu->opr, gcu->src_height, gcu->src_width, gcu->dst_width, gcu->dst_height, 0);				
			
		}else if((dst->height <= src->width)&&(dst->width <= src->height)){
			/* decimate, should dicimate first, then rotate */
			gcu->resizetype = DECIMATE_ROTATE;

			/* We only need malloc a internal decimated buffer with size: dst_height x dst_width */
			//if(gcu_new_rot_buffer(gcu, gcu->src_height, gcu->src_width)<0){
			if(gcu_new_rot_buffer(gcu, gcu->dst_width, gcu->dst_height)<0){
				puts("create rot_buf failed");
				goto error;
			}


			gcu->resize_times = gcu_create_op(gcu->rot_opr, gcu->src_width, gcu->src_height, gcu->dst_height, gcu->dst_width, 0);		
			gcu->rotate_times = gcu_create_op(gcu->opr, gcu->dst_height, gcu->dst_width, gcu->dst_width, gcu->dst_height, gcu->rotate_degree);				
			
		}else {
			/* rotate first, then check */
			gcu->resizetype = ROTATE_DECIMATE_STRETCH;
			
			if(gcu_new_rot_buffer(gcu, gcu->src_width, gcu->src_height)<0){
				puts("create rot_buf failed");
				goto error;
			}
			
	
			/* ROTATE: src_height -> src_width, src_width -> src_height */
			
			for(i=0; i<3; i++){
				if((dst->height <= src->width)&&(dst->width>= src->height)){
					/* first decimate src_width -> dst_height, use src_width -> src_width */
					gcu->mid_width[i] = gcu->src_height[i];
					gcu->mid_height[i] = gcu->dst_height[i];
				}else if((dst->height >= src->width)&&(dst->width<= src->height)){
					/* first decimate src_width -> dst_width, use src_height -> src_height */
					gcu->mid_width[i] = gcu->dst_width[i];
					gcu->mid_height[i] = gcu->src_width[i];
				}else{
					puts("src width do not support");
					goto error;
				}
				
			}

			if(gcu_new_mid_buffer(gcu)<0){
				puts("create mid_buf failed");
				goto error;
			}

			gcu->rotate_times = gcu_create_op(gcu->rot_opr, gcu->src_width, gcu->src_height, gcu->src_height, gcu->src_width, gcu->rotate_degree);							
			gcu->resize_times = gcu_create_op(gcu->mid_opr, gcu->src_height, gcu->src_width, gcu->mid_width, gcu->mid_height, 0);
			gcu->resize_times_2 = gcu_create_op(gcu->opr, gcu->mid_width, gcu->mid_height, gcu->dst_width, gcu->dst_height, 0);
		
			
		}
	}else if(GCU_ROTATE_180 == gcu->rotate_degree){
		/* need rotate */

		if((dst->height == src->height)&&(dst->width == src->width)){
			/* do not use buffer, just out rotate directly */
			gcu->resizetype = ROTATE_ONLY;

			gcu->rotate_times = gcu_create_op(gcu->opr, gcu->src_width, gcu->src_height, gcu->src_width, gcu->src_height, gcu->rotate_degree);
		}else if((dst->height >= src->height)&&(dst->width >= src->width)){
			/* stretch, should rotate first, then stretch, temp buffer needed by rotate */
			gcu->resizetype = ROTATE_STRETCH;
			/*gcu_new_rot_buffer will switch width and height, we switch them controversively for 180ROTATE do not need switch them. */
			if(gcu_new_rot_buffer(gcu, gcu->src_height, gcu->src_width)<0){
				puts("create rot_buf failed");
				goto error;
			}

			gcu->rotate_times = gcu_create_op(gcu->rot_opr, gcu->src_width, gcu->src_height, gcu->src_width, gcu->src_height, gcu->rotate_degree);				
			gcu->resize_times = gcu_create_op(gcu->opr, gcu->src_width, gcu->src_height, gcu->dst_width, gcu->dst_height, 0);				
		}else if((dst->width <= src->width)&&(dst->height <= src->height)){
			/* decimate, should dicimate first, then rotate */
			gcu->resizetype = DECIMATE_ROTATE;

			//if(gcu_new_rot_buffer(gcu, gcu->src_height, gcu->src_width)<0){
			/* New decimated buffer, malloc a dst size buffer to save buf size */
			if(gcu_new_rot_buffer(gcu, gcu->dst_height, gcu->dst_width)<0){
				puts("create rot_buf failed");
				goto error;
			}

			gcu->resize_times = gcu_create_op(gcu->rot_opr, gcu->src_width, gcu->src_height, gcu->dst_width, gcu->dst_height, 0);		
			gcu->rotate_times = gcu_create_op(gcu->opr, gcu->dst_width, gcu->dst_height, gcu->dst_width, gcu->dst_height, gcu->rotate_degree);				

		}else {

			/* rotate first, then check */
			gcu->resizetype = ROTATE_DECIMATE_STRETCH;
			
			if(gcu_new_rot_buffer(gcu, gcu->src_height, gcu->src_width)<0){
				puts("create rot_buf failed");
				goto error;
			}
			/* ROTATE: src_height -> src_height, src_width -> src_width */
			for(i=0; i<3; i++){
				if((dst->height <= src->height)&&(dst->width>= src->width)){
					/* first decimate src_height -> dst_height, use src_width -> src_width */
					gcu->mid_width[i] = gcu->src_width[i];
					gcu->mid_height[i] = gcu->dst_height[i];
				}else if((dst->height >= src->height)&&(dst->width<= src->width)){
					/* first decimate src_width -> dst_width, use src_height -> src_height */
					gcu->mid_width[i] = gcu->dst_width[i];
					gcu->mid_height[i] = gcu->src_height[i];
				}else{
					puts("src width do not support");
					goto error;
				}
				
			}

			if(gcu_new_mid_buffer(gcu)<0){
				puts("create mid_buf failed");
				goto error;
			}

			gcu->rotate_times = gcu_create_op(gcu->rot_opr, gcu->src_width, gcu->src_height, gcu->src_width, gcu->src_height, gcu->rotate_degree);							
			gcu->resize_times = gcu_create_op(gcu->mid_opr, gcu->src_width, gcu->src_height, gcu->mid_width, gcu->mid_height, 0);
			gcu->resize_times_2 = gcu_create_op(gcu->opr, gcu->mid_width, gcu->mid_height, gcu->dst_width, gcu->dst_height, 0);
		
			
		}
	}else{
	
		/* do not need rotate */
		
		if((dst->height == src->height)&&(dst->width == src->width)){

			/*move*/
			gcu->resizetype = MOVE_ONLY;
			
#ifdef BMM_ENABLE
  #ifdef GCU_WORKAROUND
			/* MVED work around,  since MVED & GCU will conflict */		
			if(gcu->use_srcbuf){
				/*only effective 1. input buffer is physical continuous,  2. MOVE_ONLY */
				gcu->pic_size = src->height * src->width;
				gcu->pic_size = gcu->pic_size + (gcu->pic_size>>1);

				gcu->use_workaround = TRUE;
			}
  #endif
#endif

			gcu->resize_times = gcu_create_op(gcu->opr, gcu->src_width, gcu->src_height, gcu->dst_width, gcu->dst_height, 0);		


		}else if((dst->height >= src->height)&&(dst->width>= src->width)){
			/*stretch*/
			gcu->resizetype = STRETCH_ONLY;
			gcu->resize_times = gcu_create_op(gcu->opr, gcu->src_width, gcu->src_height, gcu->dst_width, gcu->dst_height, 0);		
		}else if((dst->height <= src->height)&&(dst->width <= src->width)){
			/*decimate*/
			gcu->resizetype = DECIMATE_ONLY;
			gcu->resize_times = gcu_create_op(gcu->opr, gcu->src_width, gcu->src_height, gcu->dst_width, gcu->dst_height, 0);		
		}else{

			gcu->resizetype = DECIMATE_STRETCH;

			for(i=0; i<3; i++){
				if((dst->height <= src->height)&&(dst->width>= src->width)){
					/* first decimate src_height -> dst_height, use src_width -> src_width */
					gcu->mid_width[i] = gcu->src_width[i];
					gcu->mid_height[i] = gcu->dst_height[i];
				}else if((dst->height >= src->height)&&(dst->width<= src->width)){
					/* first decimate src_width -> dst_width, use src_height -> src_height */
					gcu->mid_width[i] = gcu->dst_width[i];
					gcu->mid_height[i] = gcu->src_height[i];
				}else{
					puts("src width do not support");
					goto error;
				}
				
			}	

			if(gcu_new_mid_buffer(gcu)<0){
				puts("create mid_buf failed");
				goto error;
			}

			gcu->resize_times = gcu_create_op(gcu->mid_opr, gcu->src_width, gcu->src_height, gcu->mid_width, gcu->mid_height, 0);
			gcu->resize_times_2 = gcu_create_op(gcu->opr, gcu->mid_width, gcu->mid_height, gcu->dst_width, gcu->dst_height, 0);

					
		}

		
		
	}

	return 0;

error:
	
	gcu_free_mid_buffer(gcu);		
	gcu_free_dst_buffer(gcu);
	gcu_free_rot_buffer(gcu);
	
	return -1;
	
}

void gcu_sync(struct gcu_info *gcu)
{

#ifdef BMM_ENABLE
  #ifdef GCU_WORKAROUND
	bmm_dma_sync();
  #endif
#endif

	m2d_sync(gcu->context);	
}



int gcu_close(struct gcu_info *gcu)
{	
	if (gcu->gcu_enable == FALSE){
		return 0;
	}
	gcu_sync(gcu);

	gcu_free_mid_buffer(gcu);
	gcu_free_rot_buffer(gcu);
	gcu_free_src_buffer(gcu);
	gcu_free_dst_buffer(gcu);
		
	if(gcu->context != NULL){			
		m2d_free_context(gcu->context);
		gcu->context = NULL;
	}
	gcu->gcu_enable = FALSE;
	return 0;
}


int gcu_process(struct gcu_info *gcu, struct v_frame *src, struct v_frame *dst)
{	
	int i = 0;		

	/* here to request any command left */

#ifdef BMM_ENABLE	
  #ifdef GCU_WORKAROUND
	/* we keep this code for debug ov2 without GCU */
	if(gcu->use_workaround){
		bmm_dma_sync();
		bmm_dma_memcpy(dst->start[0], src->start[0], gcu->pic_size);
		return 0;
	}
  #endif
#endif

	//m2d_sync(gcu->context);

	gcu_new_src_buffer(gcu, src);
	gcu_new_dst_buffer(gcu, dst);

	for (i=0; i<3; i++){		

		switch(gcu->resizetype)
		{
		case DECIMATE_ONLY:       		
			//puts("decimate directly");	
			
			m2d_set_srcbuf0(gcu->context, gcu->src_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->dst_buf[i]);	
		
			if(!gcu->resize_times){
				m2d_decimate_blt(gcu->context, &gcu->opr[i][0], gcu->dst_width[i], gcu->dst_height[i]);
			}else{
				m2d_decimate_blt(gcu->context, &gcu->opr[i][0], gcu->dst_width[i]/2, gcu->dst_height[i]);
				m2d_decimate_blt(gcu->context, &gcu->opr[i][1], gcu->dst_width[i]/2, gcu->dst_height[i]);	
			}
			break;

		case MOVE_ONLY:       		
			//puts("MOVE_ONLY ");	
			
			m2d_set_srcbuf0(gcu->context, gcu->src_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->dst_buf[i]);	
		
			if(!gcu->resize_times){
				m2d_raster_blt(gcu->context, &gcu->opr[i][0], 0xcc);
			}else{
				m2d_raster_blt(gcu->context, &gcu->opr[i][0], 0xcc);
				m2d_raster_blt(gcu->context, &gcu->opr[i][1], 0xcc);				
			}			

			break;

		case STRETCH_ONLY:       		
			//puts("stretch directly");	
			
			m2d_set_srcbuf0(gcu->context, gcu->src_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->dst_buf[i]);	
			
			if(!gcu->resize_times){
				m2d_stretch_blt(gcu->context, &gcu->opr[i][0], gcu->dst_width[i], gcu->dst_height[i]);
			}else{
				m2d_stretch_blt(gcu->context, &gcu->opr[i][0], gcu->dst_width[i]/2, gcu->dst_height[i]);
				m2d_stretch_blt(gcu->context, &gcu->opr[i][1], gcu->dst_width[i]/2, gcu->dst_height[i]);
			}
			break;

		case DECIMATE_STRETCH:
			//puts("DECIMATE_STRETCH");
			
			//decimate
			m2d_set_srcbuf0(gcu->context, gcu->src_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->mid_buf[i]);	

			if(!gcu->resize_times){
				m2d_decimate_blt(gcu->context, &gcu->mid_opr[i][0], gcu->mid_width[i], gcu->mid_height[i]);
			}else{
				m2d_decimate_blt(gcu->context, &gcu->mid_opr[i][0], gcu->mid_width[i]/2, gcu->mid_height[i]);
				m2d_decimate_blt(gcu->context, &gcu->mid_opr[i][1], gcu->mid_width[i]/2, gcu->mid_height[i]);	
			}			

			//stretch
			m2d_set_srcbuf0(gcu->context, gcu->mid_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->dst_buf[i]);	
			
			if(!gcu->resize_times_2){
				m2d_stretch_blt(gcu->context, &gcu->opr[i][0], gcu->dst_width[i], gcu->dst_height[i]);
			}else{
				m2d_stretch_blt(gcu->context, &gcu->opr[i][0], gcu->dst_width[i]/2, gcu->dst_height[i]);
				m2d_stretch_blt(gcu->context, &gcu->opr[i][1], gcu->dst_width[i]/2, gcu->dst_height[i]);
			}			
			
			break;
			
		case ROTATE_ONLY:
			
			m2d_set_srcbuf0(gcu->context, gcu->src_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->dst_buf[i]);	
			
			if(!gcu->rotate_times){
				m2d_rotate_blt(gcu->context, &gcu->opr[i][0], gcu->rotate_degree);
			}else{
				m2d_rotate_blt(gcu->context, &gcu->opr[i][0], gcu->rotate_degree);
				m2d_rotate_blt(gcu->context, &gcu->opr[i][1], gcu->rotate_degree);
			}
			
			break;

		case ROTATE_STRETCH:
			//puts("ROTATE_STRETCH");
			
			//rotate
			m2d_set_srcbuf0(gcu->context, gcu->src_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->rot_buf[i]);	
			
			if(!gcu->rotate_times){
				m2d_rotate_blt(gcu->context, &gcu->rot_opr[i][0], gcu->rotate_degree);
			}else{
				m2d_rotate_blt(gcu->context, &gcu->rot_opr[i][0], gcu->rotate_degree);
				m2d_rotate_blt(gcu->context, &gcu->rot_opr[i][1], gcu->rotate_degree);
			}

			//stretch
			m2d_set_srcbuf0(gcu->context, gcu->rot_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->dst_buf[i]);	
			
			if(!gcu->resize_times){
				m2d_stretch_blt(gcu->context, &gcu->opr[i][0], gcu->dst_width[i], gcu->dst_height[i]);
			}else{
				m2d_stretch_blt(gcu->context, &gcu->opr[i][0], gcu->dst_width[i]/2, gcu->dst_height[i]);
				m2d_stretch_blt(gcu->context, &gcu->opr[i][1], gcu->dst_width[i]/2, gcu->dst_height[i]);
			}			
			
			break;

		case DECIMATE_ROTATE:
			//puts("DECIMATE_ROTATE");

			//decimate
			m2d_set_srcbuf0(gcu->context, gcu->src_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->rot_buf[i]);	
		
			if(!gcu->resize_times){
				if(GCU_ROTATE_180 != gcu->rotate_degree){
					m2d_decimate_blt(gcu->context, &gcu->rot_opr[i][0], gcu->dst_height[i], gcu->dst_width[i]);
				}else{
					m2d_decimate_blt(gcu->context, &gcu->rot_opr[i][0], gcu->dst_width[i], gcu->dst_height[i]);
				}
			}else{
				if(GCU_ROTATE_180 != gcu->rotate_degree){
					m2d_decimate_blt(gcu->context, &gcu->rot_opr[i][0], gcu->dst_height[i]/2, gcu->dst_width[i]);
					m2d_decimate_blt(gcu->context, &gcu->rot_opr[i][1], gcu->dst_height[i]/2, gcu->dst_width[i]);
				}else{
					m2d_decimate_blt(gcu->context, &gcu->rot_opr[i][0], gcu->dst_width[i]/2, gcu->dst_height[i]);
					m2d_decimate_blt(gcu->context, &gcu->rot_opr[i][1], gcu->dst_width[i]/2, gcu->dst_height[i]);
				}
			}

			//rotate
			m2d_set_srcbuf0(gcu->context, gcu->rot_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->dst_buf[i]);	

			if(!gcu->rotate_times){
				m2d_rotate_blt(gcu->context, &gcu->opr[i][0], gcu->rotate_degree);
			}else{
				m2d_rotate_blt(gcu->context, &gcu->opr[i][0], gcu->rotate_degree);
				m2d_rotate_blt(gcu->context, &gcu->opr[i][1], gcu->rotate_degree);
			}
			break;

		case ROTATE_DECIMATE_STRETCH:
			//puts("ROTATE_DECIMATE_STRETCH");

			//rotate
			m2d_set_srcbuf0(gcu->context, gcu->src_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->rot_buf[i]);	

			if(!gcu->rotate_times){
				m2d_rotate_blt(gcu->context, &gcu->rot_opr[i][0], gcu->rotate_degree);
			}else{
				m2d_rotate_blt(gcu->context, &gcu->rot_opr[i][0], gcu->rotate_degree);
				m2d_rotate_blt(gcu->context, &gcu->rot_opr[i][1], gcu->rotate_degree);
			}

			//decimate
			m2d_set_srcbuf0(gcu->context, gcu->rot_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->mid_buf[i]);

			if(!gcu->resize_times){
				m2d_decimate_blt(gcu->context, &gcu->mid_opr[i][0], gcu->mid_width[i], gcu->mid_height[i]);
			}else{
				m2d_decimate_blt(gcu->context, &gcu->mid_opr[i][0], gcu->mid_width[i]/2, gcu->mid_height[i]);
				m2d_decimate_blt(gcu->context, &gcu->mid_opr[i][1], gcu->mid_width[i]/2, gcu->mid_height[i]);	
			}

			//stretch
			m2d_set_srcbuf0(gcu->context, gcu->mid_buf[i]);
			m2d_set_dstbuf2(gcu->context, gcu->dst_buf[i]);

			if(!gcu->resize_times_2){
				m2d_stretch_blt(gcu->context, &gcu->opr[i][0], gcu->dst_width[i], gcu->dst_height[i]);
			}else{
				m2d_stretch_blt(gcu->context, &gcu->opr[i][0], gcu->dst_width[i]/2, gcu->dst_height[i]);
				m2d_stretch_blt(gcu->context, &gcu->opr[i][1], gcu->dst_width[i]/2, gcu->dst_height[i]);
			}

			break;

		default:
			break;
		}
	}
	
		m2d_submit(gcu->context);	
	
	return 0;	
}

void gcu_set_rot_degree(struct gcu_info *gcu, int deg)
{
	gcu->rotate_degree = deg;
}

int gcu_get_rot_degree(struct gcu_info *gcu)
{
	return gcu->rotate_degree;
}

void gcu_set_use_srcbuf(struct gcu_info *gcu, int val)
{
	gcu->use_srcbuf = val;
}

int gcu_get_use_srcbuf(struct gcu_info *gcu)
{
	return gcu->use_srcbuf;
}




