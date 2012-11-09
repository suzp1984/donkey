#ifndef _INI_BUILDER_H
#define _INI_BUILDER_H

#include "typedef.h"

DECLES_BEGIN

struct _INIBuilder;
typedef struct _INIBuilder INIBuilder;

typedef void (*INIBuilderOnMainKeyFunc)(INIBuilder* thiz, const char* mainkey);
typedef void (*INIBuilderOnSubKeyFunc)(INIBuilder* thiz, const char* subkey);
typedef void (*INIBuilderOnValueFunc)(INIBuilder* thiz, const char* value);
typedef void (*INIBuilderOnCommentFunc)(INIBuilder* thiz, const char* comment);
typedef void (*INIBuilderOnErrorFunc)(INIBuilder* thiz, int line, int row, const char* message);
typedef void (*INIBuilderDestroyFunc)(INIBuilder* thiz);

struct _INIBuilder {
	INIBuilderOnMainKeyFunc on_main_key;
	INIBuilderOnSubKeyFunc on_sub_key;
	INIBuilderOnValueFunc on_value;
	INIBuilderOnCommentFunc on_comment;
	INIBuilderOnErrorFunc on_error;
	INIBuilderDestroyFunc destroy;

	char priv[1];
};

static inline void ini_builder_on_mainkey(INIBuilder* thiz, const char* mainkey)
{
	return_if_fail(thiz != NULL && thiz->on_main_key != NULL);

	thiz->on_main_key(thiz, mainkey);
	return;
}

static inline void ini_builder_on_subkey(INIBuilder* thiz, const char* subkey)
{
	return_if_fail(thiz != NULL && thiz->on_sub_key != NULL);

	thiz->on_sub_key(thiz, subkey);
	return;
}

static inline void ini_builder_on_value(INIBuilder* thiz, const char* value)
{
	return_if_fail(thiz != NULL && thiz->on_value != NULL);

	thiz->on_value(thiz, value);
	return;
}

static inline void ini_builder_on_comment(INIBuilder* thiz, const char* comment)
{
	return_if_fail(thiz != NULL && thiz->on_comment != NULL);

	thiz->on_comment(thiz, comment);
	return;
}

static inline void ini_builder_on_error(INIBuilder* thiz, int line, int row, const char* message)
{
	return_if_fail(thiz != NULL && thiz->on_error != NULL);

	thiz->on_error(thiz, line, row, message);
	return;
}

static inline void ini_builder_destroy(INIBuilder* thiz) 
{
	return_if_fail(thiz != NULL && thiz->destroy != NULL);

	thiz->destroy(thiz);
	return;
}

DECLES_END

#endif // _INI_BUILDER_H
