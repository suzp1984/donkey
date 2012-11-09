all: ringbuffer p_test

ringbuffer:
	gcc -Wall -g -DRINGBUFFER_TEST ringbuffer.c -o ringbuffer_test

p_test:
	gcc -Wall -g ringbuffer.c point_test.c -o point_test

