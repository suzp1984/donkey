/*
 * File:   ringbuffer_float_unfilled.h
 * Author: zpcat <suzp1984@gmail.com>
 * Brief:  filled/unfilled ringbuffer to store float
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
 * 2011-01-16 zpcat <suzp1984@gmail.com> created
 *
 */
#ifndef RINGBUFFER_FLOAT_H
#define RINGBUFFER_FLOAT_H

DECLS_BEGIN
#include "ringbuffer.h"

RingBuffer* ringbuffer_float_create(int size);

DECLS_END
#endif
