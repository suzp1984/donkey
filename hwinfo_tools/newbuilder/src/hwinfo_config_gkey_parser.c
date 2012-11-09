#include "hwinfo_config_gkey_parser.h"
#include "hwinfo.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
//#include <error.h>

typedef struct {
	GKeyFile* keyfile;

} PrivInfo;

static Ret hwinfo_gkey_parser_parser(HwinfoParser* thiz, const char* file_name, DList** items_list)
{
	return_val_if_fail(thiz != NULL && file_name != NULL, RET_INVALID_PARAMS);

	PrivInfo* priv = (PrivInfo*)thiz->priv;
	GError* err = NULL;

	g_key_file_set_list_separator(priv->keyfile, ',');

	if (!g_key_file_load_from_file(priv->keyfile, file_name, 0, &err)) {
		g_error("Parsing %s failed: %s", file_name, err->message);
		g_error_free(err);
		return RET_FAIL;
	}

	char** groups;
	int i;
	int j;

	groups = g_key_file_get_groups(priv->keyfile, NULL);

	for (i = 0; groups[i] != NULL; i++) {
		char** keys;
		keys = g_key_file_get_keys(priv->keyfile, groups[i], NULL, NULL);

		for (j = 0; keys[j] != NULL; j++) {
			char* value = g_key_file_get_value(priv->keyfile, groups[i], keys[j], NULL);

			HwinfoItem* item = (HwinfoItem*)malloc(sizeof(HwinfoItem));
			item->mainkey = strdup(groups[i]);
			item->subkey = strdup(keys[j]);
			item->value = strdup(value);

			dlist_append(*items_list, item);
		}
	}

	return RET_OK;
}

static void hwinfo_gkey_parser_destroy(HwinfoParser* thiz)
{
	return_if_fail(thiz != NULL);

	PrivInfo* priv = (PrivInfo*)thiz->priv;
	if (priv->keyfile != NULL) {
		g_key_file_free(priv->keyfile);
	}

	SAFE_FREE(thiz);

	return;
}

//TODO:XXX init g_key_file in construct
HwinfoParser* hwinfo_gkey_parser_create() 
{
	HwinfoParser* thiz;

	if ((thiz = (HwinfoParser*)malloc(sizeof(HwinfoParser) + sizeof(PrivInfo))) != NULL) {
		PrivInfo* priv = (PrivInfo*)thiz->priv;

		thiz->parser = hwinfo_gkey_parser_parser;
		thiz->destroy = hwinfo_gkey_parser_destroy;

		priv->keyfile = g_key_file_new();
	}

	return thiz;
}


#ifdef HWINFO_CONFIG_GKEY_PARSER_TEST

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


static void hwinfo_data_destroy(void* ctx, void* data) {
	return_if_fail(data != NULL);

	HwinfoItem* item = (HwinfoItem*)data;

	SAFE_FREE(item->mainkey);
	SAFE_FREE(item->subkey);
	SAFE_FREE(item->value);

	free(item);
	return;
}

static int get_main_key_count(DList* items_list)
{
	int main_key_count = 0;

	Iterator* forward = dlist_iterator_create(items_list);

	do {
		Iterator* backward;
		iterator_clone(forward, &backward);
		HwinfoItem* forword_item;
		iterator_get(forward, (void**)&forword_item);

		while (iterator_prev(backward) == RET_OK) {
			HwinfoItem* back_item;
			iterator_get(backward, (void**)&back_item);

			//printf("\t#### backward offset is %d\n", iterator_offset(backward));
			if(!strcmp(forword_item->mainkey, back_item->mainkey)) {
				goto NOCOUNT;
			}
		} 

		main_key_count++;
NOCOUNT:
		iterator_destroy(backward);

		//printf("#### forward offset is %d\n", iterator_offset(forward));
	} while (iterator_next(forward) == RET_OK);

	iterator_destroy(forward);

	return main_key_count;
}

int main()
{
	DList* items_list = dlist_create(hwinfo_data_destroy, NULL);
	HwinfoParser* hwinfo_parser =  hwinfo_gkey_parser_create();

	hwinfo_parser_config(hwinfo_parser, "./main.conf", &items_list);

	dlist_foreach(items_list, data_visit_func, NULL);

	Iterator* iterator = dlist_iterator_create(items_list);

	do {
		HwinfoItem* item;
		iterator_get(iterator, (void**)&item);

		printf("\t(main_key): %s, (sub_key): %s, (value): %s\n", item->mainkey, item->subkey, item->value);
	} while(iterator_next(iterator) == RET_OK);

	iterator_destroy(iterator);

	printf("********* main_key_count is %d\n", get_main_key_count(items_list));

	hwinfo_parser_destroy(hwinfo_parser);
	dlist_destroy(items_list);

	return 0;
}

#endif //HWINFO_CONFIG_SPRD_PARSER_TEST
