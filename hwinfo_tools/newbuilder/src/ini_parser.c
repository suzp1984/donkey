#include "ini_parser.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

struct _INIParser {
	char* mainkey_start;
	char* subkey_start;
	char* value_start;
	char* comment_start;
	char* tmp;

	char* buf;

	INIBuilder* builder;
};

static const char* strtrim(char* str);
static char* value_join(char* value_head, char* slice);
static void ini_parser_mainkey(INIParser* thiz);
static void ini_parser_subkey(INIParser* thiz);
static void ini_parser_value(INIParser* thiz);
static void ini_parser_comment(INIParser* thiz);
static void ini_parser_error(INIParser* thiz, int line, int row, char* message);

static void ini_parser_mainkey(INIParser* thiz)
{
	return_if_fail(thiz != NULL && thiz->builder != NULL);
	
	ini_builder_on_mainkey(thiz->builder, thiz->mainkey_start);
	return;
}

static void ini_parser_subkey(INIParser* thiz)
{
	return_if_fail(thiz != NULL && thiz->builder != NULL);

	ini_builder_on_subkey(thiz->builder, thiz->subkey_start);
	return;
}

static void ini_parser_value(INIParser* thiz)
{
	return_if_fail(thiz != NULL && thiz->builder != NULL);

	ini_builder_on_value(thiz->builder, thiz->value_start);
	return;
}

static void ini_parser_comment(INIParser* thiz)
{
	return_if_fail(thiz != NULL && thiz->builder != NULL);

	ini_builder_on_comment(thiz->builder, thiz->comment_start);
	return;
}

static void ini_parser_error(INIParser* thiz, int line, int row, char* message)
{
	return_if_fail(thiz != NULL && thiz->builder != NULL);

	ini_builder_on_error(thiz->builder, line, row, message);
	return;
}

const char* strtrim(char* str)
{
	char* p = NULL;
	
	p = str + strlen(str) - 1;

	while (p != str && isspace(*p))
	{
		*p = '\0';
		p--;
	}

	p = str;
	while (*p != '\0' && isspace(*p)) p++;

	if (p != str)
	{
		char* s = p;
		char* d = str;
		while (*s != '\0')
		{
			*d = *s;
			d++;
			s++;
		}
		*d = '\0';
	}

	p = str;
	while (*p != '\0') {
		if (isspace(*p)) {
			*p = ' ';
		}
		p++;
	}

	return str;
}

char* value_join(char* value_head, char* slice)
{
	char* h = value_head;
	char* s = slice;
	
	while (*h != '\0') {
		h++;
	}

	h--;
	while (h != value_head && isspace(*h)) {
		h--;
	}

	while (*s != '\0' && isspace(*s)) {
		s++;
	}

	h++;
	while (*s != '\0') {
		*h = *s;
		h++;
		s++;
	}
	*h = '\0';

	return value_head;
}

INIParser* ini_parser_create(void) {
	INIParser* thiz = (INIParser*)malloc(sizeof(INIParser));

	return thiz;
}

void ini_parser_set_builder(INIParser* thiz, INIBuilder* builder)
{
	return_if_fail(thiz != NULL && builder != NULL);

	thiz->builder = builder;
	return;
}

Ret ini_parser_load_from_file(INIParser* thiz, const char* filename, char comment_char, char delim_char)
{
	return_val_if_fail(thiz != NULL && filename != NULL, RET_INVALID_PARAMS);

	struct stat fstat;
	char* buf;
	int fd;

	if (stat(filename, &fstat)) {
		printf("Can't read the stat of config file -- %s\n", filename);
		return RET_FAIL;
	}

	buf = (char*)malloc(fstat.st_size);
	if ((fd = open(filename, O_RDONLY)) < 0) {
		return RET_FAIL;
	}

	read(fd, buf, fstat.st_size);

	return ini_parser_load_from_buf(thiz, buf, comment_char, delim_char);
}

Ret ini_parser_load_from_buf(INIParser* thiz, char* buf, char comment_char, char delim_char)
{
	return_val_if_fail(thiz != NULL && buf != NULL, RET_INVALID_PARAMS);

	thiz->buf = buf;

	enum _State 
	{
		STATE_NONE,
		STATE_MAIN_KEY,
		STATE_SUB_KEY,
		STATE_VALUE,
		STATE_VALUE_OR_KEY,
		STATE_COMMENT,
		STATE_COMMENT_AFTER_NONE
	} state = STATE_NONE;

	char* p = buf;

	for (p = buf; *p != '\0'; p++)
	{
		switch(state) 
		{
			case STATE_NONE:
			{
				if (*p == '[') {
					state = STATE_MAIN_KEY;
					thiz->mainkey_start = p + 1;
				} else if (*p == comment_char) {
					state = STATE_COMMENT_AFTER_NONE;
					thiz->comment_start = p + 1;
				} else if (!isspace(*p)) {
					state = STATE_SUB_KEY;
					thiz->subkey_start = p;
				}

				break;
			}
			case STATE_COMMENT_AFTER_NONE:
			{
				if (*p == '\n') {
					*p = '\0';
					state = STATE_NONE;
					strtrim(thiz->comment_start);
					ini_parser_comment(thiz);
				}
				
				break;
			}
			case STATE_COMMENT:
			{
				if (*p == '\n') {
					*p = '\0';
					state =  STATE_VALUE_OR_KEY;
					thiz->tmp = p + 1;
					strtrim(thiz->comment_start);
					ini_parser_comment(thiz);
				}
				break;
			}
			case STATE_MAIN_KEY:
			{
				if (*p == ']')
				{
					*p = '\0';
					state = STATE_NONE;
					strtrim(thiz->mainkey_start);
					ini_parser_mainkey(thiz);
				}
				break;
			} 
			case STATE_VALUE_OR_KEY:
			{
				if (*p == '\n') {
					*p = '\0';
					value_join(thiz->value_start, thiz->tmp);
					thiz->tmp = p + 1;
				} else if (*p == comment_char) {
					*p = '\0';
					value_join(thiz->value_start, thiz->tmp);
					state = STATE_COMMENT;
					thiz->comment_start = p + 1;
				} else if (*p == delim_char) {
					*p = '\0';
					thiz->subkey_start = thiz->tmp;
					// handle value event here
					strtrim(thiz->value_start);
					ini_parser_value(thiz);
					// handle subkey event here
					strtrim(thiz->subkey_start);
					ini_parser_subkey(thiz);
					
					state = STATE_VALUE;
					thiz->value_start = p + 1;
				} else if (*p == '[') {
					//handle value maybe
					strtrim(thiz->value_start);
					ini_parser_value(thiz);
					state = STATE_MAIN_KEY;
					thiz->mainkey_start = p + 1;
				}

				break;
			}
			case STATE_SUB_KEY:
			{
				if (*p == delim_char) {
					*p = '\0';
					state = STATE_VALUE;
					thiz->value_start = p + 1;
					strtrim(thiz->subkey_start);
					// handle subkey event
					ini_parser_subkey(thiz);
				}

				break;
			}
			case STATE_VALUE:
			{
				if (*p == '\n') {
					*p = '\0';
					thiz->tmp = p + 1;
					state = STATE_VALUE_OR_KEY;
				} else if (*p == comment_char) {
					*p = '\0';
					thiz->comment_start = p + 1;
					state = STATE_COMMENT;
				}
				break;
			}

			default: break;
		}
	}

	if (state == STATE_VALUE) {
		strtrim(thiz->value_start);
		ini_parser_value(thiz);
	} else if (state == STATE_COMMENT) {
		strtrim(thiz->comment_start);
		ini_parser_comment(thiz);
	} else if (state == STATE_VALUE_OR_KEY) {
		strtrim(thiz->value_start);
		ini_parser_value(thiz);
	}

	return RET_OK;
}

char** ini_parser_get_groups(INIParser* thiz)
{
	//char** groups = (char**)malloc(sizeof(char*));
	//groups[0] = NULL;

	return_val_if_fail(thiz != NULL, NULL);

	//return groups;
	return NULL;
}

char** ini_parser_get_keys(INIParser* thiz, const char* group)
{
	return_val_if_fail(thiz != NULL && group != NULL, NULL);

	return NULL;
}

char* ini_parser_get_value(INIParser* thiz, const char* group, const char* key)
{
	return_val_if_fail(thiz != NULL && group != NULL && key != NULL, NULL);

	return NULL;
}

void ini_parser_destroy(INIParser* thiz)
{
	return_if_fail(thiz != NULL);

	SAFE_FREE(thiz->buf);
	thiz->mainkey_start = NULL;
	thiz->subkey_start = NULL;
	thiz->value_start = NULL;
	thiz->comment_start = NULL;
	thiz->tmp = NULL;

	SAFE_FREE(thiz);

	return;
}

#ifdef INI_PARSER_TEST

#include <stdio.h>
#include "ini_builder_list.h"
#include "hwinfo.h"

#define TEST_FILE "./main.conf"

static Ret data_visit_func(void* ctx, void* data)
{
	return_val_if_fail(data != NULL, RET_FAIL);

	HwinfoItem* item = (HwinfoItem*)data;

	printf("[mainkey]: %s\n\t [subkey]: %s\n\t [value]: %s\n", item->mainkey, item->subkey, item->value);

	return RET_OK;
}

int main(int argc, char** argv)
{
	char** groups;
	int i;
	int j;
	DList* items_list;

	printf("\n************ ini parser test *************\n");
	INIParser* parser = ini_parser_create();
	INIBuilder* builder = ini_builder_list_create();
	ini_parser_set_builder(parser, builder);
	ini_parser_load_from_file(parser, TEST_FILE, '#', '=');

	items_list = ini_builder_get_list(builder);
	dlist_foreach(items_list, data_visit_func, NULL);

	/*
	groups = ini_parser_get_groups(parser);

	for (i = 0; groups[i] != NULL; i++) {
		char** keys;
		keys = ini_parser_get_keys(parser, groups[i]);

		for (j = 0; keys[j] != NULL; j++) {
			char* value = ini_parser_get_value(parser, groups[i], keys[j]);

			printf("[%s]: %s = %s\n", groups[i], keys[j], value);
		}
	} */

	ini_builder_destroy(builder);

	ini_parser_destroy(parser);

	return 0;
}

#endif // INI_PARSER_TEST
