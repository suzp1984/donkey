#include <ctype.h>
#include <string.h>
#include "hwinfo_utils.h"

int is_value_dec(const char* str)
{
	int i = 0;

	while(str[i]) {
		if(!isdigit(str[i])) {
			return UTILS_FALSE;
		}
		i++;
	}
	return UTILS_TRUE;
}

int is_value_hex(const char* str)
{
	int i;

	if(strlen(str) < 3) {
		return UTILS_FALSE;
	}

	if (str[0] != '0' && str[1] != 'x') {
		return UTILS_FALSE;
	}

	i = 2;
	while(str[i]) {
		if(!((str[i] >= '0' && str[i] <= '9') ||
					(str[i] >= 'a' && str[i] <= 'f') ||
					(str[i] >= 'A' && str[i] <= 'F'))) {
			return UTILS_FALSE;
		}
		i++;
	}

	return UTILS_TRUE;

}

gboolean is_value_string(const gchar* str)
{
	if (g_str_has_prefix(str, "\"") && g_str_has_suffix(str, "\"")) {
		return TRUE;
	} else {
		return FALSE;
	}
}

gboolean is_value_digits(const gchar* str)
{
	gchar** digits;

	digits = g_strsplit(str, ",", 0);

	while(*digits) {
		gchar* item = g_strchug(g_strchomp(*digits));

		if (!is_value_hex(item) && !is_value_dec(item)) {
			return FALSE;
		}

		digits++;
	}

	return TRUE;
}

gsize get_digit_size(const gchar* str)
{
	gchar** digits;
	gsize words = 0;

	digits = g_strsplit(str, ",", 0);

	while(*digits) {
		digits++;
		words++;
	}

	return words * 2;
}


gsize get_value_size(const gchar* value)
{
	gsize value_size;

	if (is_value_string(value)) {
		value_size = strlen(value) - 1;
		return value_size % 2 ? 2 * (value_size / 2) + 2 : value_size;
	} else if (is_value_digits(value)) {
		return get_digit_size(value);
	}

	return 0;
}

static gint hexstr2dec(const gchar* hex)
{
	gint dec = 0;
	gint size = strlen(hex);
	gint i;

	for (i = 0; i < size; i++) {
		gint m = 0;
		gchar c = hex[size - i - 1];
		if (c >= '0' && c <= '9') {
			m = c - '0';
		} else if (c >= 'A' && c <= 'F') {
			m = c - 'A' + 10;
		} else if (c >= 'a' && c <= 'f') {
			m = c - 'a' + 10;
		}

		if (i == 0) {
			dec += m;
		} else {
			dec += i * 16 * m;
		}
	}

	//printf("%s: convert %s to %d\n", __func__, hex, dec);
	return dec;
}
	                    
static gsize copy_hex_item(gchar* buffer, const gchar* value)
{
	gint i;
	gsize bytes_count;
	gsize value_size = strlen(value) - 2;
	gchar tempstr[3];
	gint tempint;

	if (value_size % 2) {
		bytes_count = value_size / 2 + 1;
	} else {
		bytes_count = value_size / 2;
	}

	if (bytes_count > 2) {
		bytes_count = 2;
	}

	for (i = 0; i < bytes_count; i++) {
		memset(tempstr, 0, 3);
		tempint = 0;

		if (value_size % 2 && value_size < 4 && (i+1) == bytes_count) {
			memcpy(tempstr, (char*)(value + 2), 1);
		} else {
			memcpy(tempstr, (char*)(value + strlen(value) - 2*(i + 1)), 2);
		}

		tempint = hexstr2dec(tempstr);
		memcpy((char*)(buffer + i), (void*)&tempint, 1);
	}

	return value_size;
}
static int copy_str(char* buffer, const char* value)
{
	int value_size;
	value_size = strlen(value) - 1;
	if (value_size % 2) {
		value_size = 2*(value_size / 2) + 2;
	}

	memcpy((gchar*)(buffer), value + 1, strlen(value) - 2);

	return value_size;
}

static int copy_digits(char* buffer, const char* value)
{
	int value_size = get_digit_size(value);

	gchar** digits;
	gsize count = 0;

	digits = g_strsplit(value, ",", 0);

	while(*digits) {
		gchar* item = g_strchug(g_strchomp(*digits));

		if (is_value_hex(item)) {
			copy_hex_item((gchar*)(buffer + count*2), item);
		} else if (is_value_dec(item)) {
			int decint = atoi(item);
			memcpy((gchar*)(buffer + count*2), &decint, 2);
		}

		digits++;
		count++;
	}

	return value_size;
}

int copy_value(char* buffer, const char* value)
{
	if (buffer == NULL || value == NULL) {
		return -1;
	}

	if (is_value_string(value)) {
		return copy_str(buffer, value);
	} else if (is_value_digits(value)) {
		return copy_digits(buffer, value);
	} else {
		printf("Wrong value: %s\n", value);
	}
	return 0;
}
