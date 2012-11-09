#ifndef TEST_CASE_H
#define TEST_CASE_H

#include "typedef.h"
#include "common.h"

#define TEST_CASE_NAME_LEN 50

struct _TestCase;
typedef struct _TestCase TestCase;

typedef int (*TestCaseRun)(TestCase* thiz);
typedef void (*TestCaseDestroy)(TestCase* thiz);

// TODO add a id member
struct _TestCase {
	TestCaseRun run;
	TestCaseDestroy destroy;

	int passed;
	const char* name;
	char priv[1];
};

static inline int test_case_run(TestCase* thiz)
{
	return_val_if_fail(thiz != NULL && thiz->run != NULL, -1);

	return thiz->run(thiz);
}

static inline int test_case_get_status(TestCase* thiz)
{
	return_val_if_fail(thiz != NULL, -1);

	return thiz->passed;
}

static inline Ret test_case_get_name(TestCase* thiz, const char** name)
{
	return_val_if_fail(thiz != NULL && thiz->name != NULL, RET_INVALID_PARAMS);

	*name = thiz->name;
	return RET_OK;
}

static inline void test_case_destroy(TestCase* thiz)
{
	return_if_fail(thiz != NULL && thiz->destroy != NULL);

	return thiz->destroy(thiz);
}

#endif // TEST_CASE_H
