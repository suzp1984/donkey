#ifndef TEST_CASE_MANAGER_H
#define TEST_CASE_MANAGER_H

#include "test_case.h"
#include "fk_config.h"
//#include "fk_config_expat_xml.h"

#include "typedef.h"

struct _TestCaseManager;
typedef struct _TestCaseManager TestCaseManager;

TestCaseManager* test_case_manager_create(FkConfig* config);

TestCase* test_case_manager_get_by_id(TestCaseManager* thiz, int id);
TestCase* test_case_manager_get_by_name(TestCaseManager* thiz, const char* name);
size_t test_case_manager_case_count(TestCaseManager* thiz);

void test_case_manager_destroy(TestCaseManager* thiz);

#endif // TEST_CASE_MANAGER_H
