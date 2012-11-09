#include "ringbuffer.h"

struct data_t {
	int x;
	int y;
	int z;
};

int main()
{
	int i = 0;
	int buffer_size = 0;
	struct data_t data;

	RingBuffer* thiz = ringbuffer_create(NULL, 5, NULL);
	data.x = 1;
	data.y = 2;
	data.z = 3;
	ringbuffer_append(thiz, (RingBuffer*)data);

	ringbuffer_destroy(thiz);
	
	return 0;
}
