#ifndef FK_GLOBALS_H
#define FK_GLOBALS_H

#include "test_case_manager.h"
#include "test_case.h"
#include "fk_config.h"

FkConfig* fk_default_config(void);
TestCaseManager* fk_default_test_case_manager(void);

void fk_set_config(FkConfig* config);
void fk_set_test_case_manager(TestCaseManager* manager);

#endif // FK_GLOBALS_H
