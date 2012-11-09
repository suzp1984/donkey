#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "typedef.h"

struct _RingBuffer;
typedef struct _RingBuffer RingBuffer;

RingBuffer* ring_buffer_create(DataDestroyFunc destroy, int size, void* ctx);

Ret ring_buffer_resize(RingBuffer* thiz, int newSize);

int ring_buffer_length(RingBuffer* thiz);
int ring_buffer_is_full(RingBuffer* thiz);
Ret ring_buffer_append(RingBuffer* thiz, void* data);
Ret ring_buffer_pop(RingBuffer* thiz,  void** data);
Ret ring_buffer_foreach(RingBuffer* thiz, DataVisitFunc visit, void* ctx);
Ret ring_buffer_clean(RingBuffer* thiz);

void ring_buffer_destroy(RingBuffer* thiz);

#endif // RING_BUFFER_H
