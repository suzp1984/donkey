#ifndef _SUM_TIME_H
#define _SUM_TIME_H

#ifdef __cplusplus
extern "C"{
#endif


//#define STATS_DEBUG


typedef struct timeval TimeVal;

void sum_init(void);
void sum_destroy(void);


void sum_lock(void);
void sum_unlock(void);

void sum_set_parameter(int name, int value);
int sum_all_rect(
		struct copybit_image_t *dst,
		struct copybit_image_t *src,
		struct copybit_rect_t *dst_rect,
		struct copybit_rect_t *src_rect,
		struct copybit_region_t *region);


#ifdef STATS_DEBUG
#define sum_start(pStartTime)	\
	start_sum(__FILE__, __LINE__, pStartTime)

#define sum_end(pStartTime, pEndTime, nUseCase)		\
	end_sum(__FILE__, __LINE__, pStartTime, pEndTime, nUseCase)

void start_sum(char * szFile, int nLine, TimeVal * pStartTime);
void end_sum(char * szFile, int nLine, TimeVal * pStartTime, TimeVal * pEndTime, int nUseCase);

#else
#define sum_start(pStartTime)	\
	start_sum(pStartTime)

#define sum_end(pStartTime, pEndTime, nUseCase)		\
	end_sum(pStartTime, pEndTime, nUseCase)

void start_sum(TimeVal * pStartTime);
void end_sum(TimeVal * pStartTime, TimeVal * pEndTime, int nUseCase);

#endif



#ifdef __cplusplus
}
#endif

#endif
