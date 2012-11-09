#ifndef _INI_BUILDER_SPRD_H
#define _INI_BUILDER_SPRD_H

#include "typedef.h"
#include "dlist.h"
#include "ini_builder.h"

DECLES_BEGIN

INIBuilder* ini_builder_list_create(void);
DList* ini_builder_get_list(INIBuilder* thiz);

DECLES_END

#endif // _INI_BUILDER_SPRD_H
