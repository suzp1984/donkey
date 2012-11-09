/*
 * File:   ringbuffer.c
 * Author: zpcat <suzp1984@gmail.com>
 * Brief:  ringbuffer impl 
 *
 *
 * Copyright (c) zpcat <suzp1984@gmail.com>
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * History:
 * ================================================================
 * 2011-01-25 zpcat <suzp1984@gmail.com> created
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "ringbuffer.h"

struct _RingBuffer {
	void** data;
	//sensors_data_t* g_buffer_data;
	size_t max_size;
	size_t curser;
	int isfull;

	void* data_destroy_ctx;
	DataDestroyFunc data_destroy;
};


RingBuffer* ringbuffer_create(DataDestroyFunc func, size_t maxSize, void* ctx)
{
	RingBuffer* thiz = (RingBuffer*)malloc(sizeof(RingBuffer));
	thiz->data = (void**)malloc(sizeof(void*)*maxSize);
	
	if((thiz != NULL) && (thiz->data != NULL)) {
		thiz->max_size = maxSize;
		thiz->isfull = FALSE;
		thiz->curser = 0;

		thiz->data_destroy = func;
	}

	return thiz;
}


Ret ringbuffer_append(RingBuffer* thiz, void* data)
{
	return_val_if_fail(thiz != NULL && data != NULL, RET_INVALID_PARAMS);

	/**
	if(thiz->curser == -1)
	{
		thiz->curser = 0;
	}**/

	// free the target data
	if(thiz->data_destroy != NULL && thiz->isfull == TRUE) {
		thiz->data_destroy(NULL, thiz->data[thiz->curser]);
	}

	/*
	if(thiz->data[thiz->curser])
	{
		free(thiz->data[thiz->curser]);
	} */

	thiz->data[thiz->curser] = data;
	if(thiz->curser < (thiz->max_size-1)) {
		thiz->curser++;
	} else if(thiz->curser == (thiz->max_size-1)) {
		thiz->isfull = TRUE;
		thiz->curser = 0;
	}

	return RET_OK;
}


Ret ringbuffer_clean(RingBuffer* thiz)
{
	int i = 0;
	int size = 0;
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	if(thiz->data_destroy != NULL) {
		//do destroy data
		if(thiz->isfull)
			size = thiz->max_size;
		else
			size = thiz->curser;

		for(i=0; i < size; i++) {
			thiz->data_destroy(NULL, thiz->data[i]);
		}
	}

	thiz->curser = 0;
	thiz->isfull = FALSE;

	return RET_OK;
}

Ret ringbuffer_foreach(RingBuffer* thiz, DataVisitFunc visit, void* ctx)
{
	int i = 0;
	int begin = 0;
	return_val_if_fail(thiz != NULL && visit != NULL, RET_INVALID_PARAMS);

	if(thiz->isfull) {
		/*
		if(thiz->curser == 0)
			begin = thiz->max_size - 1;
		else
			begin = thiz->curser - 1;
		*/
		begin = thiz->curser;
		
		for(i=begin; i < thiz->max_size; i++) {
			visit(ctx, thiz->data[i]);
		}

		for(i = 0; i < begin; i++) {
			visit(ctx, thiz->data[i]);
		}
	}else{
		for(i = 0; i < thiz->curser; i++) {
			visit(ctx, thiz->data[i]);
		}
	}

	return RET_OK;
}

Ret ringbuffer_pop_last(RingBuffer* thiz, void** data)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	if(thiz->curser == 0 && thiz->isfull == FALSE) {
		return RET_FAIL;
	}

	if(thiz->curser == 0) {
		*data = thiz->data[thiz->max_size-1];
	}else{
		*data = thiz->data[thiz->curser-1];
	}
	
	return RET_OK;
}

void ringbuffer_destroy(RingBuffer* thiz)
{
	return_if_fail(thiz != NULL);

	ringbuffer_clean(thiz);
	free(thiz->data);
	free(thiz);

	return;
}

#ifdef RINGBUFFER_TEST

typedef struct _Data_T {
	int a;
	int b;
	char c;
} Data_T;

Ret PrintVisit(void* ctx, void* data)
{
	Data_T* tmpdata = (Data_T*)data;

	printf("the output of a is %d \n", tmpdata->a);
	printf("the output of b is %d \n", tmpdata->b);
	printf("the output of c is %c \n", tmpdata->c);
	printf("----------------------\n");
	
	return RET_OK;
}

void DataDestory(void* ctx, void* data)
{
	free(data);
}

int main()
{
	int i = 0;
	Data_T* tmp_data;
	Data_T* tmp_data2;

	/*
	RingBuffer* thiz = ringbuffer_create(NULL, 11, NULL);
	for(i=0; i < 100; i++) {
		ringbuffer_append(thiz, (void*)i);
	}

	ringbuffer_foreach(thiz, PrintVisit, NULL);
	ringbuffer_clean(thiz);

	printf("unfull buffer test\n");
	for(i=0; i < 5; i++) {
		ringbuffer_append(thiz, (void*)i);
	}
	
	ringbuffer_foreach(thiz, PrintVisit, NULL);
	ringbuffer_clean(thiz);

	//critical test
	printf("critical test\n");
	for(i=0; i < 11; i++) {
		ringbuffer_append(thiz, (void*)i);
	}

	ringbuffer_foreach(thiz, PrintVisit, NULL);

	ringbuffer_destroy(thiz);
	*/

	RingBuffer* thiz = ringbuffer_create(DataDestory, 11, NULL);
	for(i=0; i < 100; i++) {
		tmp_data = (Data_T*)malloc(sizeof(Data_T));
		tmp_data->a = i;
		tmp_data->b = i + 1;
		tmp_data->c = 'a';
		ringbuffer_append(thiz, (void*)tmp_data);
	}

	ringbuffer_foreach(thiz, PrintVisit, NULL);
	ringbuffer_clean(thiz);

	printf("unfull buffer test!\n");
	for(i = 0; i < 5; i++) {
		tmp_data = (Data_T*)malloc(sizeof(Data_T));
		tmp_data->a = i;
		tmp_data->b = i + 1;
		tmp_data->c = 'a';
		ringbuffer_append(thiz, (void*)tmp_data);
	}
	ringbuffer_foreach(thiz, PrintVisit, NULL);
	ringbuffer_clean(thiz);

	printf("critical test!\n");
	for(i=0; i < 11; i++) {
		tmp_data = (Data_T*)malloc(sizeof(Data_T));
		tmp_data->a = i;
		tmp_data->b = i + 1;
		tmp_data->c = 'b';
		ringbuffer_append(thiz, (void*)tmp_data);
	}
	ringbuffer_foreach(thiz, PrintVisit, NULL);


	if(ringbuffer_pop_last(thiz, (void**)&tmp_data2) == RET_OK) {
		printf("the last data.a is %d\n", tmp_data2->a);
		printf("the last data.b is %d\n", tmp_data2->b);
		printf("the last data.c is %c\n", tmp_data2->c);
	}

	ringbuffer_destroy(thiz);

	return 0;
}

#endif
