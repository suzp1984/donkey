#include "hwinfo_build_sprd.h"
#include "hwinfo.h"
#include "hwinfo_utils.h"
#include "iterator.h"
#include "dlist_iterator.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_NAMES_BUFFER_SIZE 1024

typedef struct _PrivInfo {
	unsigned short main_key_count;
	unsigned short sub_key_count;
	unsigned short names_buffer_size;
	unsigned short value_buffer_size;
	unsigned short buffer_size;

	char* main_key_buffer;
	char* sub_key_buffer;
	char* names_buffer;
	char* value_buffer;
	char* buffer;
	
	unsigned short value_offset;

	DList* items_list;
} PrivInfo;


/*
 * hwinfo_build_sprd funcs
 */
static void hwinfo_build_sprd_on_destroy(HwinfoBuilder* thiz) {
	return_if_fail(thiz != NULL);

	PrivInfo* priv = (PrivInfo*)thiz->priv;

	SAFE_FREE(priv->main_key_buffer);
	SAFE_FREE(priv->sub_key_buffer);
	SAFE_FREE(priv->names_buffer);
	SAFE_FREE(priv->value_buffer);
	SAFE_FREE(priv->buffer);
	
	SAFE_FREE(thiz);
	return;
}

static Ret hwinfo_build_sprd_set_subkey_count(HwinfoBuilder* thiz)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);
	PrivInfo* priv = (PrivInfo*)thiz->priv;
	int sub_key_count = 0;

	Iterator* iterator = dlist_iterator_create(priv->items_list);

	do {
		sub_key_count++;
	} while (iterator_next(iterator) == RET_OK);

	priv->sub_key_count = sub_key_count;
	iterator_destroy(iterator);

	return RET_OK;
}

static Ret hwinfo_build_sprd_set_mainkey_count(HwinfoBuilder* thiz)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);
	PrivInfo* priv = (PrivInfo*)thiz->priv;
	int main_key_count = 0;

	Iterator* forward = dlist_iterator_create(priv->items_list);

	do {
		Iterator* backward;
		iterator_clone(forward, &backward);
		HwinfoItem* forword_item;
		iterator_get(forward, (void**)&forword_item);

		while (iterator_prev(backward) == RET_OK) {
			HwinfoItem* back_item;
			iterator_get(backward, (void**)&back_item);

			if(!strcmp(forword_item->mainkey, back_item->mainkey)) {
				goto NOCOUNT;
			}
		} 

		main_key_count++;
NOCOUNT:
		iterator_destroy(backward);

	} while (iterator_next(forward) == RET_OK);

	priv->main_key_count = main_key_count;
	iterator_destroy(forward);

	return RET_OK;
}

static Ret hwinfo_build_copy_name(HwinfoBuilder* thiz, const char* name, unsigned short* offset)
{
	return_val_if_fail(thiz != NULL && name != NULL && 
			offset != NULL, RET_INVALID_PARAMS);

	PrivInfo* priv = (PrivInfo*)thiz->priv;

	int name_len = strlen(name);
	char* str = NULL;

	if (priv->names_buffer != NULL) {
		str = strstr(priv->names_buffer, name);
	}

	if (str == NULL) {
		priv->names_buffer = (char*)realloc(priv->names_buffer, priv->names_buffer_size + name_len);
		if (priv->names_buffer == NULL) {
			return RET_FAIL;
		}

		memcpy((void*)(priv->names_buffer + priv->names_buffer_size), name, name_len);
		*offset = (unsigned short)priv->names_buffer_size;

		priv->names_buffer_size += name_len;
	} else {
		*offset = (unsigned short)(str - priv->names_buffer);
	}

	return RET_OK;
}

static Ret hwinfo_build_set_valuebuffer_size(HwinfoBuilder* thiz)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);
	PrivInfo* priv = (PrivInfo*)thiz->priv;
	int value_buffer_size = 0;

	Iterator* iterator = dlist_iterator_create(priv->items_list);

	do {
		HwinfoItem* item;
		iterator_get(iterator, (void**)&item);
		value_buffer_size += get_value_size(item->value);
	} while (iterator_next(iterator) == RET_OK);

	priv->value_buffer_size = value_buffer_size;
	iterator_destroy(iterator);

	return RET_OK;
}

// get number of subkeys in same main_key zone 
static int hwinfo_build_get_mainkey_subkey_count(HwinfoBuilder* thiz, HwinfoItem* item) 
{
	return_val_if_fail(thiz != NULL && item != NULL, 0);
	PrivInfo* priv = (PrivInfo*)thiz->priv;

	int count = 0;
	Iterator* iterator = dlist_iterator_create(priv->items_list);

	do {
		HwinfoItem* temp;
		iterator_get(iterator, (void**)&temp);
		if (!strcmp(temp->mainkey, item->mainkey)) {
			count++;
		}
	} while(iterator_next(iterator) == RET_OK);

	return count;
}

static Ret hwinfo_build_sprd_set(HwinfoBuilder* thiz, HwinfoItem* item)
{
	return_val_if_fail(thiz != NULL && item != NULL, RET_INVALID_PARAMS);
	PrivInfo* priv = (PrivInfo*)thiz->priv;

	int i;
	int j;
	int subkey_offset = 0;
	hwinfo_main_key_t* main_head;
	hwinfo_sub_key_t* sub_head;
	int sub_key_count = hwinfo_build_get_mainkey_subkey_count(thiz, item);

	// main key head copy
	for (i = 0; i < priv->main_key_count; i++) {
		main_head = (hwinfo_main_key_t*)(priv->main_key_buffer + 
				i * sizeof(hwinfo_main_key_t));

		if ((main_head->main_name[0] == 0 && main_head->main_name[1] == 0) 
				|| main_head->count == 0) {
			main_head->main_name[1] = strlen(item->mainkey);
			hwinfo_build_copy_name(thiz, item->mainkey, &(main_head->main_name[0]));
			main_head->offset = subkey_offset;
			main_head->count = sub_key_count;
			break;
		}

		if (!strncmp((char*)(priv->names_buffer + main_head->main_name[0]), 
				item->mainkey, strlen(item->mainkey))) {
			//main_head->count++;
			break;
		}

		subkey_offset = main_head->offset + main_head->count * sizeof(hwinfo_sub_key_t);
	}

	for (j = 0; j < sub_key_count; j++) {
		sub_head = (hwinfo_sub_key_t*)(priv->sub_key_buffer + main_head->offset + 
				j * sizeof(hwinfo_sub_key_t));

		if (sub_head->sub_name[0] == 0 && sub_head->sub_name[1] == 0) {
			sub_head->sub_name[1] = strlen(item->subkey);
			hwinfo_build_copy_name(thiz, item->subkey, &(sub_head->sub_name[0]));
			sub_head->offset = priv->value_offset;
			sub_head->words = get_value_size(item->value) / 2;
			copy_value((priv->value_buffer + sub_head->offset), 
					item->value);
			priv->value_offset += sub_head->words * 2;

			break;
		}
	}

	return RET_OK;
}

static void hwinfo_build_sprd_set_offset(HwinfoBuilder* thiz)
{
	return_if_fail(thiz != NULL);
	PrivInfo* priv = (PrivInfo*)thiz->priv;

	hwinfo_main_key_t* main_head;
	hwinfo_sub_key_t* sub_head;
	int i = 0;
	int j = 0;

	int main_key_buffer_size = priv->main_key_count * sizeof(hwinfo_main_key_t);
	int sub_key_buffer_size = priv->sub_key_count * sizeof(hwinfo_sub_key_t);
	//main_key_buffer
	for (i = 0; i < priv->main_key_count; i++) {
		main_head = (hwinfo_main_key_t*)(priv->main_key_buffer + i * sizeof(hwinfo_main_key_t));
		main_head->offset += sizeof(hwinfo_head_t) + main_key_buffer_size;
		main_head->main_name[0] += sizeof(hwinfo_head_t) + main_key_buffer_size 
			+ sub_key_buffer_size;
	}

	//sub_key_buffer
	for (j = 0; j < priv->sub_key_count; j++) {
		sub_head = (hwinfo_sub_key_t*)(priv->sub_key_buffer + j * sizeof(hwinfo_sub_key_t));
		sub_head->offset += sizeof(hwinfo_head_t) + main_key_buffer_size 
			+ sub_key_buffer_size + priv->names_buffer_size;
		sub_head->sub_name[0] += sizeof(hwinfo_head_t) + main_key_buffer_size
			+ sub_key_buffer_size;
	}

	return;
}

static Ret hwinfo_build_sprd_get_product(HwinfoBuilder* thiz, void** product, void* ctx) 
{
	return_val_if_fail(thiz != NULL && product != NULL, RET_INVALID_PARAMS);

	PrivInfo* priv = (PrivInfo*)thiz->priv;
	int buffer_size;

	//pre build
	hwinfo_build_sprd_set_mainkey_count(thiz);
	hwinfo_build_sprd_set_subkey_count(thiz);
	hwinfo_build_set_valuebuffer_size(thiz);

	priv->main_key_buffer = malloc(priv->main_key_count * sizeof(hwinfo_main_key_t));
	priv->sub_key_buffer = malloc(priv->sub_key_count * sizeof(hwinfo_sub_key_t));
	priv->value_buffer = malloc(priv->value_buffer_size);

	memset(priv->main_key_buffer, 0, priv->main_key_count * sizeof(hwinfo_main_key_t));
	memset(priv->sub_key_buffer, 0, priv->sub_key_count * sizeof(hwinfo_sub_key_t));
	memset(priv->value_buffer, 0, priv->value_buffer_size);
	
	Iterator* forward = dlist_iterator_create(priv->items_list);

	do {
		HwinfoItem* item;
		iterator_get(forward, (void**)&item);

		hwinfo_build_sprd_set(thiz, item);

	} while (iterator_next(forward) == RET_OK);

	hwinfo_build_sprd_set_offset(thiz);

	int main_key_buffer_size = priv->main_key_count * sizeof(hwinfo_main_key_t);
	int sub_key_buffer_size = priv->sub_key_count * sizeof(hwinfo_sub_key_t);

	buffer_size = sizeof(hwinfo_head_t) + main_key_buffer_size + sub_key_buffer_size
			+ priv->names_buffer_size + priv->value_buffer_size;

	priv->buffer = malloc(buffer_size);
	priv->buffer_size = buffer_size;

	hwinfo_head_t* hw_head = (hwinfo_head_t*)priv->buffer;
	hw_head->magic = MAGIC_NUM;
	hw_head->version[0] = VERSION0;
	hw_head->version[1] = VERSION1;
	hw_head->size = buffer_size;
	hw_head->main_key_count = priv->main_key_count;
		
	// copy main_key_buffer
	memcpy((priv->buffer + sizeof(hwinfo_head_t)), priv->main_key_buffer, 
			main_key_buffer_size);
	//copy sub_key_buffer
	memcpy((priv->buffer + sizeof(hwinfo_head_t) + main_key_buffer_size), priv->sub_key_buffer, 
			sub_key_buffer_size);
	//copy names_buffer
	memcpy((priv->buffer + sizeof(hwinfo_head_t) + main_key_buffer_size + sub_key_buffer_size), 
			priv->names_buffer, priv->names_buffer_size);
	//copy value_buffer
	memcpy((priv->buffer + sizeof(hwinfo_head_t) + main_key_buffer_size + sub_key_buffer_size +
				priv->names_buffer_size), priv->value_buffer, priv->value_buffer_size);

	*product = priv->buffer;
	*(int*)ctx = priv->buffer_size;

	return RET_OK;
}

HwinfoBuilder* hwinfo_build_sprd_create(DList* items_list) 
{
	return_val_if_fail(items_list != NULL, NULL);
	HwinfoBuilder* thiz;

	if ((thiz = (HwinfoBuilder*)malloc(sizeof(HwinfoBuilder) + sizeof(PrivInfo))) != NULL) {
		PrivInfo *priv = (PrivInfo*)thiz->priv;

		thiz->get_product = hwinfo_build_sprd_get_product;
		thiz->destroy = hwinfo_build_sprd_on_destroy;

		priv->main_key_count = 0;
		priv->sub_key_count = 0;
		priv->names_buffer_size = 0;
		priv->value_buffer_size = 0;
		priv->value_offset = 0;
		priv->buffer_size = 0;

		priv->main_key_buffer = NULL;
		priv->sub_key_buffer = NULL;
		priv->names_buffer = NULL;
		priv->value_buffer = NULL;
		priv->buffer = NULL;

		priv->items_list = items_list;
	}

	return thiz;
}


