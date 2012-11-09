#include "fk_globals.h"

struct _FkGlobals
{
	FkConfig* config;
	TestCaseManager* manager;
} fk_globals;

FkConfig* fk_default_config(void)
{
	return fk_globals.config;
}

TestCaseManager* fk_default_test_case_manager(void)
{
	return fk_globals.manager;
}

void fk_set_config(FkConfig* config)
{
	fk_globals.config = config;
}

void fk_set_test_case_manager(TestCaseManager* manager)
{
	fk_globals.manager = manager;
}
