#include "typedef.h"
#include "dlist.h"

#ifndef HWINFO_CONFIG_PARSER_H
#define HWINFO_CONFIG_PARSER_H

DECLES_BEGIN

struct _HwinfoParser;
typedef struct _HwinfoParser HwinfoParser;

typedef Ret (*HwinfoParserConfig)(HwinfoParser* thiz, const char* file_name, DList** items_list);
typedef void (*HwinfoParserDestroy)(HwinfoParser* thiz);

struct _HwinfoParser {
	HwinfoParserConfig parser;
	HwinfoParserDestroy destroy;

	char priv[1];
};

static inline Ret hwinfo_parser_config(HwinfoParser* thiz, const char* file_name, DList** items_list) {
	return_val_if_fail(thiz != NULL && thiz->parser != NULL && file_name != NULL, RET_INVALID_PARAMS);

	return thiz->parser(thiz, file_name, items_list);
}

static inline void hwinfo_parser_destroy(HwinfoParser* thiz) {
	return_if_fail(thiz != NULL && thiz->destroy != NULL);

	return thiz->destroy(thiz);
}

DECLES_END

#endif //HWINFO_CONFIG_PARSER_H
