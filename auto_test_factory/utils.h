#ifndef AUTO_TEST_UTILS_H
#define AUTO_TEST_UTILS_H

#include "typedef.h"

DECLES_BEGIN

int start_service(const char* name);
int stop_service(const char* name);

int open_input(const char* dev_name, int mode);

DECLES_END

#endif // AUTO_TEST_UTILS_H
