#include <glib.h>
#include <glib/gprintf.h>
#include "hwinfo_config_parser.h"
#include "hwinfo_config_gkey_parser.h"
#include "hwinfo_config_ini_parser.h"
#include "hwinfo_build.h"
#include "hwinfo_build_sprd.h"
#include "hwinfo.h"
#include "dlist.h"
#include "iterator.h"
#include "dlist_iterator.h"

#define DEFAULT_CONFIG "./main.conf"
#define DEFAULT_OUTPUT "./broncho.bin"

static gchar* config_file = NULL;
static gchar* output_file = NULL;

static void hwinfo_data_destroy(void* ctx, void* data) {
	return_if_fail(data != NULL);

	HwinfoItem* item = (HwinfoItem*)data;

	SAFE_FREE(item->mainkey);
	SAFE_FREE(item->subkey);
	SAFE_FREE(item->value);

	free(item);
	return;
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
	GError* err = NULL;

	void* buffer;
	int buffer_size;

	option_context = g_option_context_new("build hwinfo bin file from config file");
	g_option_context_add_main_entries(option_context, entries, NULL);

	if (!g_option_context_parse(option_context, &argc, &argv, &err)) {
		g_printf("option parsing failed: %s\n", err->message);
		exit(1);
	}

	g_option_context_free(option_context);

	if (config_file == NULL) {
		config_file = DEFAULT_CONFIG;
	}

	if (output_file == NULL) {
		output_file = DEFAULT_OUTPUT;
	}

	DList* items_list = dlist_create(hwinfo_data_destroy, NULL);
	//HwinfoParser* hwinfo_parser =  hwinfo_gkey_parser_create();
	HwinfoParser* hwinfo_parser =  hwinfo_ini_parser_create();
	if (hwinfo_parser_config(hwinfo_parser, config_file, &items_list) != RET_OK) {
		printf("hwinfo_parser_config failed\n");
		return -1;
	}

	HwinfoBuilder* hwinfo_builder = hwinfo_build_sprd_create(items_list);

	hwinfo_build_get_product(hwinfo_builder, &buffer, &buffer_size);
	
	//printf("buffer_size is %d\n", buffer_size);
	g_file_set_contents(output_file, buffer, buffer_size, NULL);

	hwinfo_build_destroy(hwinfo_builder);
	hwinfo_parser_destroy(hwinfo_parser);

	dlist_destroy(items_list);

	return 0;
}
