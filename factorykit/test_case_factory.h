#ifndef TEST_CASE_FACTORY_H
#define TEST_CASE_FACTORY_H

#include "test_case.h"

struct _TestCaseFactory;
typedef struct _TestCaseFactory TestCaseFactory;

static TestCaseFactory* factory;

TestCase* test_case_factory_get(const char* name);


#endif // TEST_CASE_FACTORY_H
