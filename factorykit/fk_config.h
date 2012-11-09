#ifndef FK_CONFIG_H
#define FK_CONFIG_H

#include "typedef.h"

struct _FkConfig;
typedef struct _FkConfig FkConfig;

typedef Ret (*FkConfigLoad)(FkConfig* thiz, const char* config_file);
typedef size_t (*FkConfigGetCasesCount)(FkConfig* thiz);
typedef Ret (*FkConfigGetCasesNameById)(FkConfig* thiz, int id, char** names);
typedef char** (*FkConfigGetAllCases)(FkConfig* thiz);

typedef void (*FkConfigDestroy)(FkConfig* thiz);

struct _FkConfig {
	FkConfigLoad load;
	FkConfigGetCasesCount get_cases_count;
	FkConfigGetCasesNameById get_cases_name_by_id;
	FkConfigGetAllCases get_all_cases;

	FkConfigDestroy destroy;

	char priv[1];
};

static inline Ret fk_config_load(FkConfig* thiz, const char* config_file) 
{
	return_val_if_fail(thiz != NULL &&  thiz->load != NULL 
			&& config_file != NULL, RET_INVALID_PARAMS);

	return thiz->load(thiz, config_file);
}

static inline size_t fk_config_get_cases_count(FkConfig* thiz)
{
	return_val_if_fail(thiz != NULL && thiz->get_cases_count != NULL, 0);

	return thiz->get_cases_count(thiz);
}

static inline Ret fk_config_get_cases_name_by_id(FkConfig* thiz, int id, char** items) 
{
	return_val_if_fail(thiz != NULL && thiz->get_cases_name_by_id != NULL, RET_INVALID_PARAMS);

	return thiz->get_cases_name_by_id(thiz, id, items);
}

static inline char** fk_config_get_all_cases(FkConfig* thiz)
{
	return_val_if_fail(thiz != NULL && thiz->get_all_cases != NULL, NULL);

	return thiz->get_all_cases(thiz);
}

static inline void fk_config_destroy(FkConfig* thiz)
{
	return_if_fail(thiz != NULL && thiz->destroy != NULL);

	return thiz->destroy(thiz);
}

#endif // FK_CONFIG_H

