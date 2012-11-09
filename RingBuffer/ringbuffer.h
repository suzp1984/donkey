/*
 * File:   ringerbuffer.h
 * Author: zpcat <suzp1984@gmail.com>
 * Brief:  ringbuffer header file, it is a abstraction of ringbuffer
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
 * 2011-01-13 zpcat <suzp1984@gmail.com> created
 *
 */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "typedef.h"

struct _RingBuffer;
typedef struct _RingBuffer RingBuffer;

//typedef void* (*RingBufferCreate)(int size);
typedef void (*RingBufferDestroy)(RingBuffer* thiz);
typedef Ret (*RingBufferAppend)(RingBuffer* thiz, void* data);
typedef void* (*RingBufferForeach)(RingBuffer* thiz);
typedef void (*RingBufferCleanBuffer)(RingBuffer* thiz);

struct _RingBuffer {
	//RingBufferCreate  create;
	RingBufferAppend append;
	RingBufferForeach toArray;
	RingBufferCleanBuffer clean;
	RingBufferDestroy destroy;

	char priv[1];
}

static int ringbuffer_append(RingBuffer* thiz, void* data)
{
	return_val_if_fail(thiz != NULL && thiz->append != NULL, -1);

	return thiz->append(thiz, data);
}

static void* ringbuffer_to_array(RingBuffer* thiz)
{
	return_val_if_fail(thiz != NULL && thiz->toArray != NULL, NULL);

	return thiz->toArray(thiz);
}

static void ringbuffer_destroy(RingBuffer* thiz)
{
	return_if_fail(thiz != NULL && thiz->destroy != NULL);

	return thiz->destroy(thiz);
}

#ifdef __cplusplus
}
#endif

#endif
