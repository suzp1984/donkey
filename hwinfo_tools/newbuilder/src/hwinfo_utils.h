#include <glib.h>
#include "typedef.h"

#ifndef HWINFO_UTILS_H
#define HWINFO_UTILS_H

DECLES_BEGIN

#define UTILS_FALSE 0
#define UTILS_TRUE  1

int is_value_dec(const char* str);
int is_value_hex(const char* str);
gboolean is_value_string(const gchar* str);
gboolean is_value_digits(const gchar* str);
gsize get_digit_size(const gchar* str);
gsize get_value_size(const gchar* value);
int copy_value(char* buffer, const char* value);

DECLES_END

#endif //HWINFO_UTILS_H
