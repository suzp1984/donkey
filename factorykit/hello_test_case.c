#include "hello_test_case.h"
#include <stdio.h>

#define HELLO_MESSAGE "hello world"


typedef struct {
	int status;
} PrivInfo;

static int hello_test_case_run(TestCase* thiz)
{
	//DECLES_PRIV(priv, thiz);

	printf("%s: %s\n", __func__, HELLO_MESSAGE);
	thiz->passed = 1;

	return 0;
}

static void hello_test_case_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);

	return;
}

TestCase* hello_test_case_create(int passed)
{
	TestCase* thiz = (TestCase*) malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);

		thiz->run = hello_test_case_run;
		thiz->destroy = hello_test_case_destroy;
		thiz->name = HELLO_CASE_NAME;
		thiz->passed = passed;

		priv->status = 2;
	}

	return thiz;
}

#ifdef HELLO_TEST_CASE_TEST

int main(int argc, char* argv[])
{
	int passed = -1;
	char* case_name;
	TestCase* hello = hello_test_case_create(0);
	passed = test_case_get_status(hello);
	test_case_get_name(hello, (const char**)&case_name);
	printf("name: %s, passed: %d\n", case_name, passed);

	test_case_run(hello);
	passed = test_case_get_status(hello);
	printf("name: %s, passed: %d\n", case_name, passed);

	test_case_destroy(hello);

	return 0;
}

#endif // HELLO_TEST_CASE_TEST
