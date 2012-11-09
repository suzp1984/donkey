/*
 * copybit_def.h
 *
 *  Created on: Jan 12, 2010
 */

#ifndef COPYBIT_DEF_H_
#define COPYBIT_DEF_H_


#ifdef __cplusplus
extern "C"{
#endif

#include <errno.h>
#include <hardware/copybit.h>
//#include "gcu/m2d_copybit.h"
#include "m2d_lib.h"
#include "mmx/compsurf.h"

#define NO_ERROR    0

//XXX: need keep consistent with private_handle_t(gralloc_priv.h)
#define IMAGE_FD(img)      (img)->handle->data[0]
#define IMAGE_OFFSET(img)  (img)->handle->data[4]
#define IMAGE_BASE(img)   ((img)->handle->data[5])

typedef struct copybit_context_t {
	struct copybit_device_t device;
	//broncho mod
	//struct m2d_copybit_context context;

	int mFD;
	int wmmx_pmem_fd;
	unsigned char *wmmx_pmem_buf;
	// our private state goes below here
	int mRotationDegree;
	int mPlaneAlpha;
	int mDitherEnabled;
	int mBlurEnabled;
	int mTransformation;
	int mTransformationLast;
} copybit_context_t;

/**
 * Common hardware methods
 */
static int open_device(
		const struct hw_module_t* module,
		const char* name,
		struct hw_device_t** device);

static int close_device(struct hw_device_t *dev);


static int copybit_set_parameter(
		struct copybit_device_t *dev,
		int name,
		int value);

static int copybit_get_parameter(
		struct copybit_device_t *dev,
		int name);

static int copybit_blit(
		struct copybit_device_t *dev,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_region_t const *region);

static int copybit_stretch(
		struct copybit_device_t *dev,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_region_t const *region);

//#define RGB565(r, g, b) 	( ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3) )

static int marvell_copybit(
		struct copybit_device_t *dev,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_region_t const *region);

/* broncho del gcu
extern int marvell_gcu_copybit(
		copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *rect,
		uint8_t global_alpha);
*/

extern int marvell_mmx_copybit(
		struct copybit_context_t *ctx,
		struct copybit_image_t const *dst,
		struct copybit_image_t const *src,
		struct copybit_rect_t const *dst_rect,
		struct copybit_rect_t const *src_rect,
		struct copybit_rect_t const *rect,
		IppBlendMode blendMode,
		Ipp8u mPlaneAlpha);

extern int copybit_get_step_size(int format);



#ifdef __cplusplus
}
#endif


#endif /* COPYBIT_DEF_H_ */
