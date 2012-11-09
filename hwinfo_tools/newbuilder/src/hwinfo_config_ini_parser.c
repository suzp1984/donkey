#include "hwinfo_config_ini_parser.h"
#include "ini_builder.h"
#include "ini_parser.h"
#include "ini_builder_list.h"
#include "dlist.h"
#include "iterator.h"
#include "dlist_iterator.h"
#include "hwinfo.h"
#include <string.h>
#include <stdio.h>

#define COMMENT_CHAR '#'
#define DELIM_CHAR '='

typedef struct {
	INIBuilder* ini_builder;
	INIParser* ini_parser;
	
} PrivInfo;

static Ret hwinfo_ini_parser_parser(HwinfoParser* thiz, const char* filename, DList** items_list)
{
	return_val_if_fail(thiz != NULL && filename != NULL && *items_list != NULL, RET_INVALID_PARAMS);

	PrivInfo* priv = (PrivInfo*)thiz->priv;
	DList* lists = NULL;

	return_val_if_fail(ini_parser_load_from_file(priv->ini_parser, filename, 
				COMMENT_CHAR, DELIM_CHAR) == RET_OK, RET_FAIL);

	lists = ini_builder_get_list(priv->ini_builder);
	if (dlist_length(lists) == 0) {
		printf("config file is empty\n");
		return RET_INVALID_PARAMS;
	}

	Iterator* forward = dlist_iterator_create(lists);

	do {
		HwinfoItem* forward_item;
		HwinfoItem* item;
		iterator_get(forward, (void**)&forward_item);
		
		item = (HwinfoItem*)malloc(sizeof(HwinfoItem));
		item->mainkey = strdup(forward_item->mainkey);
		item->subkey = strdup(forward_item->subkey);
		item->value = strdup(forward_item->value);

		dlist_append(*items_list, item);

	} while (iterator_next(forward) == RET_OK);
	
	iterator_destroy(forward);

	return RET_OK;
}

static void hwinfo_ini_parser_destroy(HwinfoParser* thiz)
{
	return_if_fail(thiz != NULL);

	PrivInfo* priv = (PrivInfo*)thiz->priv;

	ini_builder_destroy(priv->ini_builder);
	ini_parser_destroy(priv->ini_parser);

	free(thiz);
	return;
}

HwinfoParser* hwinfo_ini_parser_create()
{
	HwinfoParser* thiz = (HwinfoParser*)malloc(sizeof(HwinfoParser) + sizeof(PrivInfo));

	if (thiz != NULL)
	{
		PrivInfo* priv = (PrivInfo*)thiz->priv;
		
		thiz->parser = hwinfo_ini_parser_parser;
		thiz->destroy = hwinfo_ini_parser_destroy;

		priv->ini_parser = ini_parser_create();
		priv->ini_builder = ini_builder_list_create();
		ini_parser_set_builder(priv->ini_parser, priv->ini_builder);
	}

	return thiz;
}

#ifdef HWINFO_CONFIG_INI_PARSER_TEST

#include "dlist.h"

static Ret data_visit_func(void* ctx, void* data)
{
	return_val_if_fail(data != NULL, RET_FAIL);

	HwinfoItem* item = (HwinfoItem*)data;

	printf("[mainkey]: %s\n\t [subkey]: %s\n\t [value]: %s\n", item->mainkey, item->subkey, item->value);

	return RET_OK;
}

static void hwinfo_data_destroy(void* ctx, void* data) {
	return_if_fail(data != NULL);

	HwinfoItem* item = (HwinfoItem*)data;

	SAFE_FREE(item->mainkey);
	SAFE_FREE(item->subkey);
	SAFE_FREE(item->value);

	free(item);
	return;
}

int main(int argc, char* argv[])
{
	DList* items_list = dlist_create(hwinfo_data_destroy, NULL);

	HwinfoParser* hwinfo_ini_parser = hwinfo_ini_parser_create();
	hwinfo_parser_config(hwinfo_ini_parser, "./main.conf", &items_list);

	dlist_foreach(items_list, data_visit_func, NULL);

	hwinfo_parser_destroy(hwinfo_ini_parser);
	dlist_destroy(items_list);

	return 0;
}

#endif // HWINFO_CONFIG_INI_PARSER_TEST
