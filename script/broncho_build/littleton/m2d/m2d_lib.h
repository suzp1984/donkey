/*
 * INTEL CONFIDENTIAL
 * Copyright 2005-2006 Intel Corporation All Rights Reserved.
 * 
 * The source code contained or described herein and all documents
 * related to the source code (Material) are owned by Intel Corporation
 * or its suppliers or licensors.  Title to the Material remains with
 * Intel Corporation or its suppliers and licensors. The Material contains
 * trade secrets and proprietary and confidential information of Intel
 * or its suppliers and licensors. The Material is protected by worldwide
 * copyright and trade secret laws and treaty provisions. No part of the
 * Material may be used, copied, reproduced, modified, published, uploaded,
 * posted, transmitted, distributed, or disclosed in any way without Intel's
 * prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or
 * delivery of the Materials, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights
 * must be express and approved by Intel in writing.

 *(C) Copyright 2006 Marvell International Ltd.  
 * All Rights Reserved 
 */

#ifndef M2D_LIB_H
#define M2D_LIB_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "m2d_drv.h"

struct m2d_buff;
struct m2d_context;

enum {
	ERR_OK = 0,
	ERR_OOB,	/* Out of Boundary */
	ERR_IPF,	/* Invalid pixel format */
	ERR_REG,	/* Invalid ALU scratchpad register nr */
};

/**
 * Pixel format constant, along with step information.
 * Some pixel format may have two alternatives, packed
 * and unpacked, different in step size.
 *
 * GCU_MAKE_PXLFMT() will combine the format and step into a constant;
 * GCU_PXLFMT_STEP() and GCU_PIXEL_FORMAT() are used to extract format
 * and step value from this constant.
 */
#define GCU_MAKE_PXLFMT(f,step)	((f & 0x0f) | (step << 4))
#define GCU_PXLFMT_STEP(pf)  	((pf & 0xf0) >> 4)
#define GCU_PXLFMT_FORMAT(pf)	(pf & 0x0f)

typedef enum {
	GCU_PXLFMT_REGISTER		= -1,
	GCU_PXLFMT_INDEXED_8		= GCU_MAKE_PXLFMT(0x0, 1),
	GCU_PXLFMT_RGB555		= GCU_MAKE_PXLFMT(0x1, 2),
	GCU_PXLFMT_RGBT5551		= GCU_MAKE_PXLFMT(0x2, 2),
	GCU_PXLFMT_RGB565		= GCU_MAKE_PXLFMT(0x3, 2),
	GCU_PXLFMT_RGB666_PACKED	= GCU_MAKE_PXLFMT(0x4, 3),
	GCU_PXLFMT_RGB666		= GCU_MAKE_PXLFMT(0x4, 4),
	GCU_PXLFMT_RGBT6661_PACKED	= GCU_MAKE_PXLFMT(0x5, 3),
	GCU_PXLFMT_RGBT6661		= GCU_MAKE_PXLFMT(0x5, 4),
	GCU_PXLFMT_RGB888_PACKED	= GCU_MAKE_PXLFMT(0x6, 3),
	GCU_PXLFMT_RGB888		= GCU_MAKE_PXLFMT(0x6, 4),
	GCU_PXLFMT_RGBA6666_PACKED	= GCU_MAKE_PXLFMT(0x7, 3),
	GCU_PXLFMT_RGBA6666		= GCU_MAKE_PXLFMT(0x7, 4),
	GCU_PXLFMT_RGBT8881		= GCU_MAKE_PXLFMT(0x8, 4),
	GCU_PXLFMT_RGBA8888		= GCU_MAKE_PXLFMT(0x9, 4),
	GCU_PXLFMT_RGB161616		= GCU_MAKE_PXLFMT(0xa, 8),
	GCU_PXLFMT_RGBA16161616		= GCU_MAKE_PXLFMT(0xb, 8),
} GCU_PIXEL_FORMAT;

typedef uint64_t GCU_COLOR_VALUE;

typedef enum {
	GCU_BLEND_OP_ADD	= 0,
	GCU_BLEND_OP_SUB	= 1,
	GCU_BLEND_OP_REVERSE_SUB= 2
} GCU_BLEND_OP;

typedef enum {
	GCU_BIAS_OP_ADD		= 0,
	GCU_BIAS_OP_SUB		= 1,
} GCU_BIAS_OP;

/**
 * Note: use ALPHA as the key is invalid
 */
typedef enum {
	GCU_CKBLT_KEY_BLUE	= 0,
	GCU_CKBLT_KEY_GREEN	= 1,
	GCU_CKBLT_KEY_RED	= 2,
	GCU_CKBLT_KEY_ALPHA	= 3,
} GCU_CKBLT_KEY;

typedef enum {
	GCU_ROTATE_0		= 0,
	GCU_ROTATE_90		= 1,
	GCU_ROTATE_180		= 2,
	GCU_ROTATE_270		= 3,
} GCU_ROTATION;
/* returns the address of the graphics buffer that application can access */
void * m2d_buff_addr(struct m2d_buff *);	
size_t m2d_buff_size(struct m2d_buff *);

/**
 * @brief m2d_op_region defines the region where most operations will be
 * performed.
 * 
 * (s0x0, s0y0) defines the Left-Top corner of the operation region within
 * 		source buffer 0.
 * (s1x0, s1y0) defines the Left-Top corner of the operation region within
 * 		source buffer 1.
 * (dx0, dy0)   defines the Left-Top corner of the operation region within
 * 		current destination buffer.
 * (width, height) defines the width and height of all the three region if
 * 		they are same.
 *
 * For operations that requires different width/height of buffers, (width,
 * height) defines the source 0 region, while the different (width, height)
 * will be supplied by the function parameters.
 */
struct m2d_op_region {
	int	s0x0, s0y0;
	int	s1x0, s1y0;
	int	dx0,  dy0;
	int	width, height;
};

/* For setting operation region involves src0, src1, dst buffer */
#define SET_OP_REGION(opr, x0, y0, x1, y1, dx, dy, w, h)\
	do {						\
	(opr)->s0x0 = x0; (opr)->s0y0 = y0;		\
	(opr)->s1x0 = x1; (opr)->s1y0 = y1;		\
	(opr)->dx0  = dx; (opr)->dy0  = dy;		\
	(opr)->width = w; (opr)->height = h;		\
	} while (0)

/* For setting operation region involves only src0, dst buffer */
#define SET_OP_REGION_SRC0(opr, x0, y0, dx, dy, w, h)	\
	do {						\
	(opr)->s0x0 = x0; (opr)->s0y0 = y0;		\
	(opr)->dx0  = dx; (opr)->dy0 = dy;		\
	(opr)->width = w; (opr)->height = h;		\
	} while (0)

/* For setting operation region involves only dst buffer */
#define SET_OP_REGION_DST(opr, x, y, w, h)		\
	do {						\
	(opr)->dx0  = x;  (opr)->dy0  = y;		\
	(opr)->width = w; (opr)->height = h;		\
	} while (0)

/**
 * @brief m2d_submit() will submit the encoded instructions in the current
 * buffer to the driver for execution. This function will block if another
 * context holds the GCU, once the GCU has been unlocked, instructions in
 * the buffer will be moved to the GCU's ring buffer and returns right after
 * enabling the GCU. For this function to perform in a non-block behavior,
 * one can call m2d_context_set_mode() with NONBLOCK flags.
 */
int m2d_submit(struct m2d_context *);
int m2d_submit_nodelay(struct m2d_context *);

/**
 * @brief m2d_sync() will try to synchronize between application and GCU
 * execution, i.e, wait until GCU finishes execution. Normally, one will
 * have to call this function before getting an error status.
 */
int m2d_sync(struct m2d_context *);

/* Context Management */
struct m2d_context *m2d_create_context(void);
void m2d_free_context(struct m2d_context *);

/* Buffer Management */
/**
 * m2d_create_buffer - create a m2d_buff based on virtual address
 * of a physically contiguous memory
 *
 * m2d_destroy_buffer - destroy a m2d_buff created by the above
 * function
 */
struct m2d_buff *m2d_create_buffer(
		struct m2d_context *ctx,
		int width,
		int height,
		GCU_PIXEL_FORMAT pf,
		void *virt_addr);

void m2d_destroy_buffer(
		struct m2d_context *ctx,
		struct m2d_buff *);

struct m2d_buff *m2d_alloc_buffer(
		struct m2d_context *ctx,
		int width,
		int height,
		GCU_PIXEL_FORMAT pf);

struct m2d_buff *m2d_sub_buffer(
		struct m2d_buff *buff,
		int x, int y,
		int w, int h);

void m2d_free_buffer(
		struct m2d_context *ctx,
		struct m2d_buff *);

/* Context Management helping functions */
int m2d_load_reg(
		struct m2d_context *,
		int regnr,
		GCU_PIXEL_FORMAT pf, 
		GCU_COLOR_VALUE c);

int m2d_set_srcbuf0(struct m2d_context *, struct m2d_buff *);
int m2d_set_srcbuf1(struct m2d_context *, struct m2d_buff *);
int m2d_set_dstbuf0(struct m2d_context *, struct m2d_buff *);
int m2d_set_dstbuf1(struct m2d_context *, struct m2d_buff *);
int m2d_set_dstbuf2(struct m2d_context *, struct m2d_buff *);

#define USE_SRC_ALPHA		(0xffffffff)

int  m2d_set_fill_color(
		struct m2d_context *,
		GCU_PIXEL_FORMAT,
		GCU_COLOR_VALUE);

void m2d_set_line_width(struct m2d_context *, int width);
int  m2d_set_line_color(
		struct m2d_context *,
		GCU_PIXEL_FORMAT,
		GCU_COLOR_VALUE);

void m2d_set_aaline_width(struct m2d_context *, int width);
int  m2d_set_aaline_color(
		struct m2d_context *,
		GCU_PIXEL_FORMAT,
		GCU_COLOR_VALUE);
void m2d_set_aaline_mask (
		struct m2d_context *,
		unsigned char mask[8]);

void m2d_set_blend_op(struct m2d_context *, GCU_BLEND_OP op);
void m2d_set_bias_op (struct m2d_context *, GCU_BIAS_OP  op);
void m2d_set_ckblt_invert(struct m2d_context *, int);

GCU_BLEND_OP m2d_get_blend_op(struct m2d_context *);
GCU_BIAS_OP  m2d_get_bias_op (struct m2d_context *);
int          m2d_get_ckblt_invert(struct m2d_context *);

/* 2D Graphics Control and Memory Interface Interface */
int m2d_nop(struct m2d_context *ctx, int nop_id);

/* 2D Graphics Primitives */
int m2d_draw_line(
		struct m2d_context *ctx,
		int x0, int y0,
		int x1, int y1);

int m2d_draw_line_imm(
		struct m2d_context *ctx,
		int x0, int y0,
		int x1, int y1,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE color_or_regnr,
		int width);

int m2d_draw_aaline(
		struct m2d_context *ctx,
		int x0, int y0,
		int x1, int y1);

int m2d_draw_aaline_imm(
		struct m2d_context *ctx,
		int x0, int y0,
		int x1, int y1,
		GCU_PIXEL_FORMAT pf,
		GCU_COLOR_VALUE color_or_regnr,
		int width,
		uint8_t mask_array[8]);

int m2d_color_fill(
		struct m2d_context *ctx,
		struct m2d_op_region *opr);

int m2d_color_fill_imm(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		GCU_PIXEL_FORMAT,
		GCU_COLOR_VALUE);

/**
 * @brief m2d_stretch_blt will perform the stretch blt operation
 *
 * @param dw target width in destination buffer
 * @param dh target height in destination buffer
 */
int m2d_stretch_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		int dw,  int dh);	/* target width and height */

/**
 * @brief m2d_decimate_blt will perform the decimate blt operation
 *
 * @param dw target width in destination buffer
 * @param dh target height in destination buffer
 */
int m2d_decimate_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		int dw,  int dh);

/**
 * @brief m2d_scale_blt will perform the following blt operation:
 *
 * 	scale_factor * src0 ==> Dst
 *
 * @param scale_factor can be either directly specified or
 * USE_SRC_ALPHA to use the RGBA alpha value of the source pixels.
 *
 * scale_factor is a 16-bit number with 8-bit integer part, and 8-bit
 * fractional part.
 */
int m2d_scale_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		unsigned int scale_factor);

/**
 * @brief m2d_bias_blt will perform the following blt operation:
 *
 * 	src0 +/- bias_factor ==> dst
 *
 * @param bias_factor can be either directly specified or be
 * USE_SRC_ALPHA to use the RGBA alpha component of the source pixels.
 */
int m2d_bias_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		unsigned int bias_factor);

/**
 * @brief m2d_chroma_key_blt will perform the chroma key blt operation
 * from src0 to dst.
 *
 * @param key is the chroma color key, @see enum GCU_CHROMA_KEY
 * @param threshold is the threshold value for comparison
 */
int m2d_chroma_key_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		GCU_CKBLT_KEY key,
		unsigned int threshold);

/*
 * @brief blending operation can be performed either on add basis or
 * substract or reverse-substract basis, depending on blend_op parameter
 * in encoded instruction. This parameter will be maintained in the
 * current context, and modified by @see m2d_set_blend_op()
 */

/**
 * @brief m2d_alpha_blend_single will perform the (1 - alpha) blend operation
 * according to the following equation:
 *
 * 	alpha0 * src0 + (1 - alpha0) * src1 ==> dst
 *
 * @param alpha0 alpha co-efficiency in the above equation, 16bit unsigned
 * fractional format (Q15). This can be either specified directly or
 * USE_SRC_ALPHA to use the RGBA alpha value of the corresponding pixels
 * in source buffer 0.
 */
int m2d_alpha_blend_single(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		unsigned int alpha);

/**
 * @brief m2d_alpha_blend will perform the alpha blend operation
 * according to the following equation:
 *
 * 	alpha0 * src0 + alpha1 * src1 ==> dst
 *
 * The alpha values can be either directly specified or
 * USE_SRC_ALPHA to use the RGBA alpha values of the corresponding pixels
 * in the source buffer 0 and 1.
 */
int m2d_alpha_blend(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		unsigned int alpha0,
		unsigned int alpha1);

/**
 * @brief m2d_raster_blt will perform the raster operation, buffers involved
 * and equation depend on raster_op, and application is responsible for
 * setting everything right.
 *
 * @param raster_op raster operation type
 */
int m2d_raster_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		int raster_op);

/**
 * @brief m2d_rotate_blt will perform the roate copy blt operation. The
 * angle depends on 'rot'. See GCU_ROTATION for supported angle.
 */
int m2d_rotate_blt(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		GCU_ROTATION rot);
/**
 * @brief m2d_pattern_copy_mode0 will perform the pattern copy blt operation
 * according to option 1: It replicates a rectangular block of data from
 * source 0 prepeatedly in the destination buffer.
 */
int m2d_pattern_copy_mode1(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		int dw, int dh);	/* target width & height */

/**
 * @brief m2d_pattern_copy_mode1 will perform the pattern copy blt operation
 * according to option 2: It streams a block of data out in a linear fashion
 * (useful for texture patterns when the output should look random).
 */
int m2d_pattern_copy_mode2(
		struct m2d_context *ctx,
		struct m2d_op_region *opr,
		int dw, int dh);

struct m2d_buff *m2d_get_primary(void);

#ifdef __cplusplus
}
#endif
#endif /* M2D_LIB_H */
