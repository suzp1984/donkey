#ifndef _INI_PARSER_H
#define _INI_PARSER_H

#include "ini_builder.h"
#include "typedef.h"

DECLES_BEGIN

struct _INIParser;
typedef struct _INIParser INIParser;

INIParser* ini_parser_create(void);
void ini_parser_set_builder(INIParser* thiz, INIBuilder* builder);
Ret ini_parser_load_from_file(INIParser* thiz, const char* filename, char comment_char, char delim_char);
Ret ini_parser_load_from_buf(INIParser* thiz, char* buf, char comment_char, char delim_char);

char** ini_parser_get_groups(INIParser* thiz);
char** ini_parser_get_keys(INIParser* thiz, const char* group);
char* ini_parser_get_value(INIParser* thiz, const char* group, const char* key);
void ini_parser_destroy(INIParser* thiz);

DECLES_END

#endif //_INI_PARSER_H
