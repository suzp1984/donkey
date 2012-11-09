#include "atv_test.h"
#include "ui.h"
#include "minui.h"
#include "common.h"

#include <stdlib.h>

#define LOG_TAG "backlight"
#include <utils/Log.h>

static int atv_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	pthread_t t;

	fk_stop_service("media");

	return 0;
}

static void atv_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* atv_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase));

	if (thiz != NULL) {
		thiz->run = atv_test_run;
		thiz->destroy = atv_test_destroy;

		thiz->name = ATV_TEST_CASE;
		thiz->passed = passed;
	}

	return thiz;
}
