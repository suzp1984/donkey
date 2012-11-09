#ifndef INPUT_EVENT_READER_H
#define INPUT_EVENT_READER_H

#include <linux/input.h>
#include "typedef.h"

struct _InputEventReader;
typedef struct _InputEventReader InputEventReader;

InputEventReader* input_event_reader_create(int size);
int input_event_reader_fill(InputEventReader* thiz, int fd);
int input_event_reader_get(InputEventReader* thiz, struct input_event const** events);

void input_event_reader_destroy(InputEventReader* thiz);


#endif // INPUT_EVENT_READER_H
