#include "input_event_reader.h"
#include "typedef.h"
#include "ring_buffer.h"

struct _InputEventReader {
	RingBuffer* ringbuf;
};

InputEventReader* input_event_reader_create(int size)
{
	InputEventReader* thiz = (InputEventReader*) malloc(sizeof(InputEventReader));

	if (thiz != NULL) {
		thiz->ringbuf = ring_buffer_create(NULL, size, NULL);
	}

	return thiz;
}

int input_event_reader_fill(InputEventReader* thiz, int fd)
{
	return_val_if_fail(thiz != NULL, -1);

	return 0;
}

int input_event_reader_get(InputEventReader* thiz, struct input_event const** events)
{
	return_val_if_fail(thiz != NULL, -1);
	int num_events = 0;

	return num_events;
}

void input_event_reader_destroy(InputEventReader* thiz)
{
	return_if_fail(thiz != NULL);

	ring_buffer_destroy(thiz->ringbuf);
	SAFE_FREE(thiz);

	return;
}


#ifdef INPUT_EVENT_READER_TEST

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

int main(int argc, char* argv[])
{
	int fd;
	struct pollfd mpoll_fds[1];
	struct input_event* events;
	int num;

	InputEventReader* reader = input_event_reader_create(10);
	fd = open("/dev/input/event6", O_RDONLY, 0);
	if (fd < 0) {
		return -1;
	}

	mpoll_fds[0].fd = fd;
	mpoll_fds[0].events = POLLIN;
	mpoll_fds[0].revents = 0;

	while (1) {
		poll(mpoll_fds, 1, -1);

		if (mpoll_fds[0].revents & POLLIN) {
			input_event_reader_fill(reader, fd);
			num = input_event_reader_get(reader, &events);

			mpoll_fds[0].revents = 0;
		}
	}

	input_event_reader_destroy(reader);

	return 0;
}

#endif // INPUT_EVENT_READER_TEST

