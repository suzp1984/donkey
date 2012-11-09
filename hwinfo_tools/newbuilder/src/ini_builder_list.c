#include "ini_builder_list.h"
#include "hwinfo.h"
#include <string.h>

typedef struct _PrivInfo {
	char* main_key;
	char* sub_key;

	DList* item_list;
}PrivInfo;

static void hwinfo_data_destroy(void* ctx, void* data) {
	return_if_fail(data != NULL);

	HwinfoItem* item = (HwinfoItem*)data;

	SAFE_FREE(item->mainkey);
	SAFE_FREE(item->subkey);
	SAFE_FREE(item->value);

	free(item);
	return;
}

static void ini_builder_list_on_mainkey(INIBuilder* thiz, const char* mainkey)
{
	return_if_fail(thiz != NULL && mainkey != NULL);

	PrivInfo* priv = (PrivInfo*)thiz->priv;
	SAFE_FREE(priv->main_key);

	priv->main_key = strdup(mainkey);

	return;
}

static void ini_builder_list_on_subkey(INIBuilder* thiz, const char* subkey)
{
	return_if_fail(thiz != NULL && subkey != NULL);

	PrivInfo* priv = (PrivInfo*)thiz->priv;
	SAFE_FREE(priv->sub_key);
	
	priv->sub_key = strdup(subkey);
	return;
}

static void ini_builder_list_on_value(INIBuilder* thiz, const char* value)
{
	return_if_fail(thiz != NULL && value != NULL);

	PrivInfo* priv = (PrivInfo*)thiz->priv;

	return_if_fail(priv->main_key != NULL && priv->sub_key != NULL);

	HwinfoItem* item = (HwinfoItem*)malloc(sizeof(HwinfoItem));
	item->mainkey = strdup(priv->main_key);
	item->subkey = strdup(priv->sub_key);
	item->value = strdup(value);

	dlist_append(priv->item_list, item);

	return;
}

static void ini_builder_list_on_comment(INIBuilder* thiz, const char* comment)
{
	return_if_fail(thiz != NULL);

	return;
}

static void ini_builder_list_on_error(INIBuilder* thiz, int line, int row, const char* message)
{
	return_if_fail(thiz != NULL);

	fprintf(stderr, "(%d, %d) %s\n", line, row, message);
	return;
}

static void ini_builder_list_destroy(INIBuilder* thiz)
{
	return_if_fail(thiz != NULL);

	PrivInfo* priv = (PrivInfo*)thiz->priv;
	SAFE_FREE(priv->main_key);
	SAFE_FREE(priv->sub_key);

	dlist_destroy(priv->item_list);

	free(thiz);
}

INIBuilder* ini_builder_list_create(void)
{
	INIBuilder* thiz = (INIBuilder*)malloc(sizeof(INIBuilder) + sizeof(PrivInfo));

	if (thiz != NULL) {
		PrivInfo* priv = (PrivInfo*)thiz->priv;

		thiz->on_main_key = ini_builder_list_on_mainkey;
		thiz->on_sub_key = ini_builder_list_on_subkey;
		thiz->on_value = ini_builder_list_on_value;
		thiz->on_comment = ini_builder_list_on_comment;
		thiz->on_error = ini_builder_list_on_error;
		thiz->destroy = ini_builder_list_destroy;

		priv->item_list = dlist_create(hwinfo_data_destroy, NULL);
		priv->sub_key = NULL;
		priv->main_key = NULL;
	}

	return thiz;
}

DList* ini_builder_get_list(INIBuilder* thiz)
{
	return_val_if_fail(thiz != NULL, NULL);

	PrivInfo* priv = (PrivInfo*)thiz->priv;

	return priv->item_list;
}

#ifdef INI_BUILDER_TEST

#include "iterator.h"
#include "dlist_iterator.h"
#include <unistd.h>

static Ret data_visit_func(void* ctx, void* data)
{
	return_val_if_fail(data != NULL, RET_FAIL);

	HwinfoItem* item = (HwinfoItem*)data;

	printf("[mainkey]: %s\n\t [subkey]: %s\n\t [value]: %s\n", item->mainkey, item->subkey, item->value);

	return RET_OK;
}

int main(int argc, char* argv[])
{
	DList* items_list;
	INIBuilder* builder = ini_builder_list_create();

	ini_builder_on_mainkey(builder, "topwise");
	ini_builder_on_subkey(builder, "name");
	ini_builder_on_value(builder, "cat");
	ini_builder_on_subkey(builder, "age");
	ini_builder_on_value(builder, "2");

	ini_builder_on_mainkey(builder, "bra");
	ini_builder_on_subkey(builder, "size");
	ini_builder_on_value(builder, "M");

	items_list = ini_builder_get_list(builder);
	dlist_foreach(items_list, data_visit_func, NULL);

	ini_builder_destroy(builder);

	return 0;
}

#endif // INI_BUILDER_TEST
