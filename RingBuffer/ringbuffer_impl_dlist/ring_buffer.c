#include "ring_buffer.h"
#include "dlist.h"

#include <stdlib.h>

struct _RingBuffer {
	DList* list;

	int size;
	int length;
	int is_full;
	int head;
	int end;
};

RingBuffer* ring_buffer_create(DataDestroyFunc destroy, int size, void* ctx)
{
	RingBuffer* thiz = (RingBuffer*) malloc(sizeof(RingBuffer));

	if (thiz != NULL) {
		thiz->list = dlist_create(destroy, ctx);
		thiz->size = size;
		thiz->length = 0;
		thiz->is_full = 0;
		thiz->head = -1;
		thiz->end = -1;
	}

	return thiz;
}


Ret ring_buffer_resize(RingBuffer* thiz, int newSize)
{
	return RET_OK;
}

int ring_buffer_length(RingBuffer* thiz)
{
	return_val_if_fail(thiz != NULL, -1);

	return thiz->length;
}

int ring_buffer_is_full(RingBuffer* thiz)
{
	return_val_if_fail(thiz != NULL , -1);

	return thiz->is_full;
}

static void ring_buffer_length_increase(RingBuffer* thiz)
{
	return_if_fail(thiz != NULL);

	if (thiz->length < thiz->size) {
		thiz->length++;
	}

	if (thiz->head == -1 && thiz->end == -1) {
		thiz->head ++;
		thiz->end ++;
	} else if (thiz->is_full) {
		thiz->head = (thiz->head >= thiz->size - 1) ? 0 : thiz->head+1;
		thiz->end = (thiz->end >= thiz->size - 1) ? 0 : thiz->end+1;
	} else {
		thiz->end = (thiz->end >= thiz->size - 1) ? 0 : thiz->end+1;
	}

	if (thiz->length == thiz->size) {
		thiz->is_full = 1;
	}
	 
}

static void ring_buffer_length_decrease(RingBuffer* thiz)
{
	return_if_fail(thiz != NULL && thiz->length != 0);

	thiz->is_full = 0;
	if (thiz->length > 0) {
		thiz->length--;
	}

	if (thiz->length == 0) {
		thiz->head = -1;
		thiz->end = -1;
	} else {
		thiz->head = (thiz->head >= thiz->size - 1) ? 0 : thiz->head+1;
	}
}

Ret ring_buffer_append(RingBuffer* thiz, void* data)
{
	return_val_if_fail(thiz != NULL && data != NULL, RET_INVALID_PARAMS);

	ring_buffer_length_increase(thiz);

	if (dlist_length(thiz->list) > thiz->end  ) {
		dlist_delete(thiz->list, thiz->end);
		dlist_insert(thiz->list, thiz->end, data);
	} else {
		dlist_append(thiz->list, data);
	}

	return RET_OK;
}

Ret ring_buffer_pop(RingBuffer* thiz,  void** data)
{
	return_val_if_fail(thiz != NULL && data != NULL, RET_INVALID_PARAMS);

	if (thiz->length > 0) {
		dlist_get_by_index(thiz->list, thiz->head, data);
		ring_buffer_length_decrease(thiz);
	} else {
		return RET_FAIL;
	}

	return RET_OK;
}

Ret ring_buffer_foreach(RingBuffer* thiz, DataVisitFunc visit, void* ctx)
{
	return_val_if_fail(thiz != NULL && visit != NULL, RET_INVALID_PARAMS);

	void* data;
	int i = 0;
	if (thiz->length == 0) {
		return RET_OK;
	}

	//printf("** end: %d, head: %d\n", thiz->end, thiz->head);
	if (thiz->end >= thiz->head) {
		for (i = thiz->head; i <= thiz->end; i++) {
			dlist_get_by_index(thiz->list, i, &data);
			visit(ctx, data);
		}
	} else {
		for (i = thiz->head; i < thiz->size; i++) {
			dlist_get_by_index(thiz->list, i, &data);
			visit(ctx, data);
		}

		for (i = 0; i <= thiz->end; i++) {
			dlist_get_by_index(thiz->list, i, &data);
			visit(ctx, data);
		}
	}

	return RET_OK;
}

Ret ring_buffer_clean(RingBuffer* thiz)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	thiz->length = 0;
	thiz->head = -1;
	thiz->end = -1;
	thiz->is_full = 0;

	return RET_OK;
}

void ring_buffer_destroy(RingBuffer* thiz)
{
	return_if_fail(thiz != NULL);

	dlist_destroy(thiz->list);
	SAFE_FREE(thiz);
	return;
}

#ifdef RING_BUFFER_TEST

#include <assert.h>
#include <stdio.h>

typedef struct _Data_T {
	int a;
	int b;
	char c;
} Data_T;

Ret PrintVisit(void* ctx, void* data)
{
	/*
	Data_T* tmpdata = (Data_T*)data;

	printf("the output of a is %d \n", tmpdata->a);
	printf("the output of b is %d \n", tmpdata->b);
	printf("the output of c is %c \n", tmpdata->c);
	printf("----------------------\n");
	*/
	printf("the data is %d\n", (int)data);

	return RET_OK;
}

int main(int argc, char* argv[])
{
	RingBuffer* ring = ring_buffer_create(NULL, 10, NULL);

	printf("======= ring_buffer_append TEST ========\n");
	int m;
	int i = 0;
	for (i = 1; i < 5; i++) {
		assert(ring_buffer_append(ring, (void*)i) == RET_OK);
	}

	assert(ring_buffer_length(ring) == 4);

	ring_buffer_foreach(ring, PrintVisit, NULL);
	printf("------ring_buffer_clean TEST------------\n");
	ring_buffer_clean(ring);

	for (i = 1; i < 15; i++)
		ring_buffer_append(ring, (void*)i);

	assert(ring_buffer_length(ring) == 10);
	ring_buffer_foreach(ring, PrintVisit, NULL);

	printf("======== ring_buffer_pop TEST ========\n");

	for (i = 0; i < 3; i++)
	{
		ring_buffer_pop(ring, &m);
		printf("m: %d\n", m);
	}
	assert(ring_buffer_length(ring) == 7);

	ring_buffer_foreach(ring, PrintVisit, NULL);
	printf("-------\n");
	for (i = 1; i < 3; i++) {
		ring_buffer_append(ring, (void*)i);
	}

	for (i = 0; i < 5; i++)
	{
		ring_buffer_pop(ring, &m);
		printf("m: %d\n", m);
	}

	assert(ring_buffer_length(ring) == 4);
	ring_buffer_foreach(ring, PrintVisit, NULL);

	for (i = 0; i < 5; i++)
	{
		ring_buffer_pop(ring, &m);
		printf("m: %d\n", m);
	}

	assert(ring_buffer_length(ring) == 0);
	ring_buffer_destroy(ring);

	return 0;
}

#endif // RING_BUFFER_TEST
