#include "test_case_manager.h"
#include "test_case_factory.h"

#define LOG_TAG "factorykit"
#include <utils/Log.h>

struct _TestCaseManager {
	FkConfig* config;
	TestCase** cases;
	size_t cases_count;
};

TestCaseManager* test_case_manager_create(FkConfig* config)
{
	TestCaseManager* thiz = (TestCaseManager*) malloc(sizeof(TestCaseManager));

	if (thiz != NULL) {
		size_t i = 0;
		char* name;
		thiz->config = config;
		thiz->cases_count = fk_config_get_cases_count(config);
		thiz->cases = (TestCase**) malloc(thiz->cases_count * sizeof(TestCase*));

		for (i = 0; i < thiz->cases_count; i++) {
			fk_config_get_cases_name_by_id(thiz->config, i, &name);
			thiz->cases[i] = test_case_factory_get((const char*)name);
		}
	}

	return thiz;
}

TestCase* test_case_manager_get_by_id(TestCaseManager* thiz, int id)
{
	return_val_if_fail(thiz != NULL && id >= 0, NULL);

	return thiz->cases[id];
}

TestCase* test_case_manager_get_by_name(TestCaseManager* thiz, const char* name)
{
	return_val_if_fail(thiz != NULL && name != NULL, NULL);

	size_t i = 0;
	for (i = 0; i < thiz->cases_count; i++) {
		char* buf = NULL;
		test_case_get_name(thiz->cases[i], (const char**)&buf);
		if (!strcmp(name, buf)) {
			return thiz->cases[i];
		}
	}

	return NULL;
}

size_t test_case_manager_case_count(TestCaseManager* thiz)
{
	return_val_if_fail(thiz != NULL, -1);

	return thiz->cases_count;
}

void test_case_manager_destroy(TestCaseManager* thiz)
{
	return_if_fail(thiz != NULL);

	size_t i = 0;
	for (i = 0; i < thiz->cases_count; i++) {
		LOGE("%s: destroy case (%d)", __func__, i);
		test_case_destroy(thiz->cases[i]);
	}

	SAFE_FREE(thiz->cases);
	SAFE_FREE(thiz);
}

#ifdef TEST_CASE_MANAGER_TEST

#include "fk_config_expat_xml.h"
#include <assert.h>

#define XML_CONFIG_FILE "./test-cases.xml"

int main(int argc, char* argv[])
{
	int i = 0;
	int count = 0;
	FkConfig* config = fk_config_expat_create();

	assert(RET_OK == fk_config_load(config, XML_CONFIG_FILE));

	TestCaseManager* cases_manager = test_case_manager_create(config);
	count = test_case_manager_case_count(cases_manager);
	assert(1 == count);

	for (i = 0; i < count; i++) {
		test_case_run(test_case_manager_get_by_id(cases_manager, i));
	}

	// destroy
	test_case_manager_destroy(cases_manager);
	fk_config_destroy(config);
}

#endif // TEST_CASE_MANAGER_TEST

