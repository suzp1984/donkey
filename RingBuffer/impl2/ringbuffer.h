/*
 * File:   ringbuffer.h
 * Author: zpcat <suzp1984@gmail.com>
 * Brief:  ringbuffer interface definition
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

#include "typedef.h"

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

DECLS_BEGIN

#define TRUE 1
#define FALSE 0

struct _RingBuffer;
typedef struct _RingBuffer RingBuffer;

RingBuffer* ringbuffer_create(DataDestroyFunc func, size_t maxSize, void* ctx);

Ret ringbuffer_append(RingBuffer* ringbuffer, void* data);
Ret ringbuffer_clean(RingBuffer* ringbuffer);
Ret ringbuffer_foreach(RingBuffer* ringbuffer, DataVisitFunc visit, void* ctx);
Ret ringbuffer_pop_last(RingBuffer* ringbuffer, void** data);

void ringbuffer_destroy(RingBuffer* ringbuffer);

DECLS_END
#endif
