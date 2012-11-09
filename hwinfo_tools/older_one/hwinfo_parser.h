#ifndef _HWINFO_PARSER_H
#define _HWINFO_PARSER_H

typedef enum {
	HWINFO_PARSER_OK,
	HWINFO_PARSER_FAIL,
	HWINFO_PARSER_NOTFOUND
}Ret;

Ret hwinfo_parser_init(char *script_buf);
Ret hwinfo_parser_exit(void);
int hwinfo_parser_getcount(char* main_name, char* sub_name);
Ret hwinfo_parser_fetch(char *main_name, char *sub_name, int value[]);

#endif // _HWINFO_PARSER_H
