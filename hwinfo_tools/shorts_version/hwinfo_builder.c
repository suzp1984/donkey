#include <glib.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "hwinfo.h"

#define DEFAULT_BUFFER_SIZE 1024*1024
#define DEFAULT_CONFIG "main.conf"

static gchar* config_file;
static gchar* output_file;

static GKeyFile* load_config(const char* file)
{
	GError* err = NULL;
	GKeyFile* keyfile;

	if (file == NULL) {
		file = DEFAULT_CONFIG;
	}

	keyfile = g_key_file_new();
	g_key_file_set_list_separator(keyfile, ',');

	if (!g_key_file_load_from_file(keyfile, file, 0, &err)) {
		error("Parsing %s failed: %s", file, err->message);
		g_error_free(err);
		g_key_file_free(keyfile);
		return NULL;
	}

	return keyfile;
}

static gboolean is_value_string(const gchar* str)
{
	if (g_str_has_prefix(str, "\"") && g_str_has_suffix(str, "\"")) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static gboolean is_value_hex(const gchar* str)
{
	gint i;

	if(strlen(str) < 3) {
		return FALSE;
	}

	if (str[0] != '0' && str[1] != 'x') {
		return FALSE;
	}

	i = 2;
	while(str[i]) {
		if(!(str[i] >= '0' && str[i] <= '9' ||
					str[i] >= 'a' && str[i] <= 'f' ||
					str[i] >= 'A' && str[i] <= 'F')) {
			return FALSE;
		}
		i++;
	}

	return TRUE;

}

static gboolean is_value_dec(const gchar* str)
{
	gint i = 0;

	while(str[i]) {
		if(!isdigit(str[i])) {
			return FALSE;
		}
		i++;
	}
	return TRUE;
}

static gboolean is_value_digits(const gchar* str)
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

static gint hexstr2dec(const gchar* hex)
{
	gint dec = 0;
	gint size = strlen(hex);
	gint i;

	for (i = 0; i < size; i++) {
		gint m;
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

// return words count
static gsize copy_str(hwinfo_sub_key_t* sub_key, gchar* buffer, const gchar* value)
{
	gsize value_size;
	value_size = strlen(value) - 1;
	if (value_size % 2) {
		value_size = 2*(value_size / 2) + 2;
	} 
		
	sub_key->words = value_size / 2;

	memcpy((gchar*)(buffer + sub_key->offset), value + 1, strlen(value) - 2);

	return value_size; 
}

static gsize copy_hex(hwinfo_sub_key_t* sub_key, gchar* buffer, const gchar* value)
{
	gint i;
	gint bytes_count;
	gsize value_size = strlen(value) - 2;
	gchar tempstr[3];
	gint tempint;
	

	if (value_size % 2) {
		bytes_count = value_size / 2 + 1;
	} else {
		bytes_count = value_size / 2;
	}

	if (bytes_count % 4) {
		value_size = 4 * (bytes_count / 4) + 4;
	} else {
		value_size = bytes_count;
	}

	sub_key->words = value_size;
	// TODO
	for (i = 0; i < bytes_count; i++) {
		memset(tempstr, 0, 3);
		tempint = 0;
		if (strlen(value) % 2 && (i+1) == bytes_count) {
			memcpy(tempstr, (char*)(value + 2), 1);
		} else {
			memcpy(tempstr, (char*)(value + strlen(value) - 2*(i + 1)), 2);
			
		}

	    tempint = hexstr2dec(tempstr);
	    memcpy((char*)(buffer + sub_key->offset + i), (void*)&tempint, 1);
	}

	return value_size;
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

static gsize copy_dec(hwinfo_sub_key_t* sub_key, gchar* buffer, const gchar* value)
{
	int decint = atoi(value);

	sub_key->words = 4;
	memcpy((gchar*)(buffer + sub_key->offset), &decint, 4);

	return 4;
}

static gsize get_digit_size(const gchar* str)
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


static gsize copy_digits(hwinfo_sub_key_t* sub_key, gchar* buffer, const gchar* value) 
{
	int value_size = get_digit_size(value);

	gchar** digits; 
	gsize count = 0;

	digits = g_strsplit(value, ",", 0);
	sub_key->words = value_size / 2;

	while(*digits) {
		gchar* item = g_strchug(g_strchomp(*digits));

		if (is_value_hex(item)) {
			copy_hex_item((gchar*)(buffer + sub_key->offset + count*2), item);
		} else if (is_value_dec(item)) {
			int decint = atoi(item);
			memcpy((gchar*)(buffer + sub_key->offset + count*2), &decint, 2);
		}

		digits++;
		count++;
	}

	return value_size;

}

static gsize get_value_size(const gchar* value)
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

static GOptionEntry entries[] =
{
	{ "config", 'c', 0, G_OPTION_ARG_STRING, &config_file, "Configure file name", NULL },
	{ "output", 'o', 0, G_OPTION_ARG_STRING, &output_file, "Output file name", NULL},
	{ NULL }
};

int main(int argc, char* argv[])
{
	GOptionContext* option_context;
	GKeyFile* config;
	GError* err = NULL;
	gchar** groups;
	gchar* buffer;
	gsize sub_key_sum = 0;
	gsize value_size = 0;
	gsize buffer_size = 0;
	gsize value_offset = 0;
	gsize names_size = 0;
	gsize names_offset = 0;
	int fd;
	
	gint i = 0;

	gsize main_key_count;
	hwinfo_head_t* hw_header;

	option_context = g_option_context_new("build hwinfo bin file from config file");
	g_option_context_add_main_entries(option_context, entries, NULL);

	if (!g_option_context_parse(option_context, &argc, &argv, &err)) {
		g_printf("option parsing failed: %s\n", err->message);
		exit(1);
	}

	g_option_context_free(option_context);

	/* Step 1: parser config file */

	config = load_config(config_file);
	if(!config) return -1;

	/* Step 2: calc buffer size */

	groups = g_key_file_get_groups(config, &main_key_count);

	for ( i = 0; groups[i] != NULL; i++ ) {
		gchar** keys;
		gint j = 0;
		gsize sub_key_count;
		hwinfo_main_key_t hw_main_key;

		keys = g_key_file_get_keys(config, groups[i], &sub_key_count, NULL);
		sub_key_sum += sub_key_count;
		g_strlcpy((gchar*)hw_main_key.main_name, groups[i], 32);

		hw_main_key.count = sub_key_count;
		hw_main_key.offset = sizeof(hwinfo_head_t) + sizeof(hwinfo_main_key_t) * main_key_count + 0;
		
		for ( j = 0; keys[j] != NULL; j++ ) {
			names_size += strlen(groups[i]) + strlen(keys[j]);
			value_size += get_value_size(g_key_file_get_value(config, groups[i], keys[j], NULL));
		}
	}

	names_offset = sizeof(hwinfo_head_t) + main_key_count * sizeof(hwinfo_main_key_t) +
		sub_key_sum * sizeof(hwinfo_sub_key_t);
	value_offset = names_offset + names_size;
	buffer_size = value_offset + value_size;

	/* Step 3: builder buffer */
	buffer = (gchar*)g_malloc0(buffer_size);

	hw_header = (hwinfo_head_t*)buffer;
	hw_header->main_key_count = main_key_count;
	hw_header->size = (unsigned short)value_size;

	sub_key_sum = 0;
	value_size = 0;
	names_size = 0;
	for ( i = 0; groups[i] != NULL; i++ ) {
		gchar** keys;
		gint j = 0;
		gsize sub_key_count;
		hwinfo_main_key_t* hw_main_key = (hwinfo_main_key_t*) (buffer + sizeof(hwinfo_head_t) +
				i * sizeof(hwinfo_main_key_t));
		keys = g_key_file_get_keys(config, groups[i], &sub_key_count, NULL);
		hw_main_key->main_name[0] = names_offset + names_size;
		hw_main_key->main_name[1] = strlen(groups[i]);
		names_size += hw_main_key->main_name[1];
		g_strlcpy((gchar*)(buffer + hw_main_key->main_name[0]), groups[i], -1);
		hw_main_key->count = sub_key_count;
		hw_main_key->offset = sizeof(hwinfo_head_t) + sizeof(hwinfo_main_key_t) * main_key_count + 
			sub_key_sum * sizeof(hwinfo_sub_key_t);
		
		sub_key_sum += sub_key_count;

		// TODO: XXX aliged by short 
		for ( j = 0; keys[j] != NULL; j++ ) {
			gchar* value;
			hwinfo_sub_key_t* hw_sub_key = (hwinfo_sub_key_t*)(buffer + hw_main_key->offset + 
					j * sizeof(hwinfo_sub_key_t));
			hw_sub_key->sub_name[0] = names_offset + names_size;
			hw_sub_key->sub_name[1] = strlen(keys[j]);
			names_size += hw_sub_key->sub_name[1];
			g_strlcpy((gchar*)(buffer + hw_sub_key->sub_name[0]), keys[j], -1);
			hw_sub_key->offset = value_offset + value_size;
			value = g_key_file_get_value(config, groups[i], keys[j], NULL);
			if (is_value_string(value)) {
				value_size += copy_str(hw_sub_key, buffer, value);
			} else if (is_value_digits(value)) {
				value_size += copy_digits(hw_sub_key, buffer, value);
			} else {
				g_printf("Wrong value: %s\n", value);
				return -1;
			}
		}
	}

	/* step 4: write buffer into a file */
	g_file_set_contents((output_file != NULL) ? output_file : BIN_FILE, buffer, buffer_size, NULL);

	return 0;
}
