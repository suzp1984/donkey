/*
 * sum_data.h
 *
 *  Created on: Jan 13, 2010
 *      Author: archermind
 */

#ifndef SUM_DATA_H_
#define SUM_DATA_H_

#ifdef __cplusplus
extern "C"{
#endif

void log_copybit_start();

void log_copybit_end(
	struct copybit_context_t *ctx,
	struct copybit_image_t const *src,
	struct copybit_rect_t const *src_rect,
	struct copybit_image_t const *dst,
	struct copybit_rect_t const *dst_rect,
	struct copybit_rect_t const *rect);

#ifdef __cplusplus
}
#endif

#endif /* SUM_DATA_H_ */
