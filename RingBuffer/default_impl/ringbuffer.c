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
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	/**
	if(thiz->curser == -1)
	{
		thiz->curser = 0;
	}**/

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
		if(thiz->curser == 0)
			begin = thiz->max_size - 1;
		else
			begin = thiz->curser - 1;
		
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


void ringbuffer_destroy(RingBuffer* thiz)
{
	return_if_fail(thiz != NULL);

	free(thiz->data);
	free(thiz);

	return;
}

#ifdef RINGBUFFER_TEST

Ret PrintVisit(void* ctx, void* data)
{
	printf("the output is %d \n", (int)data);
	
	return RET_OK;
}

Ret BufferSize(void* ctx, void* data)
{
	int* count = (int*)ctx;
	(*count)++;
	
	return RET_OK;
}

int main()
{
	int i = 0;
	int buffer_size = 0;

	RingBuffer* thiz = ringbuffer_create(NULL, 11, NULL);
	for(i=0; i < 100; i++) {
		ringbuffer_append(thiz, (void*)i);
	}

	ringbuffer_foreach(thiz, PrintVisit, NULL);
	ringbuffer_foreach(thiz, BufferSize, &buffer_size);
	printf("++++the buffer size is %d\n", buffer_size);
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
	return 0;
}

#endif
