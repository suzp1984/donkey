#ifndef _HW_BUILDER_H
#define _HW_BUILDER_H

struct _hwinfo_builder;

typedef struct _hwinfo_builder hwinfo_builder;

struct _hwinfo_builder
{
	gsize main_key_count;
	gchar** main_keys;
	gchar* buffer;
	gsize buffer_size;
};

//int hwinfo_

#endif //_HW_BUILDER_H
