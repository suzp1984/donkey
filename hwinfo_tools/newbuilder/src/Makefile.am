bin_PROGRAMS=hwinfo_builder hwinfo_parser
noinst_PROGRAMS=hwinfo_gkey_parser_test hwinfo_ini_parser_test

hwinfo_builder_SOURCES = main.c \
						 hwinfo_build_sprd.c \
						 hwinfo_config_gkey_parser.c \
						 hwinfo_config_ini_parser.c \
						 ini_parser.c \
						 ini_builder_list.c \
						 hwinfo_utils.c \
						 dlist.c

hwinfo_builder_LDFLAGS = -lglib-2.0
hwinfo_builder_CFLAGS = -Wall -g  -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include

hwinfo_parser_SOURCES = hwinfo_parser.c
hwinfo_parser_CFLAGS = -Wall -g -DHWINFO_PARSER_TEST

hwinfo_gkey_parser_test_SOURCES = hwinfo_config_gkey_parser.c \
								  dlist.c 
hwinfo_gkey_parser_test_LDFLAGS = -lglib-2.0
hwinfo_gkey_parser_test_CFLAGS = -Wall -g  -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -DHWINFO_CONFIG_GKEY_PARSER_TEST

hwinfo_ini_parser_test_SOURCES = hwinfo_config_ini_parser.c \
								 ini_builder_list.c \
								 ini_parser.c \
								 dlist.c
hwinfo_ini_parser_test_CFLAGS =  -Wall -g -DHWINFO_CONFIG_INI_PARSER_TEST

EXTRA_DIST = main.conf
