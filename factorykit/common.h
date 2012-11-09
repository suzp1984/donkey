#ifndef FK_TEST_COMMON_H
#define FK_TEST_COMMON_H

#define TEST_PASSED 1
#define TEST_FAILED 0
#define TEST_UNCERTAINTY -1

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int fk_start_service(const char* name);
int fk_stop_service(const char* name);

int open_input(const char* dev_name, int mode);

#endif // FK_TEST_COMMON_H
