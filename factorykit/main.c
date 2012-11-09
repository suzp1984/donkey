#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/reboot.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#include "ui.h"
#include "minui.h"
#include "common.h"
#include "menu.h"

#include "fk_globals.h"
#include "fk_config_expat_xml.h"
#include "fk_sqlite.h"

#define ENG_TEST                    "engtest_mode"
#define FK_CMD_LEN 1024

#define XML_CONFIG_FILE "/system/oem/test-cases.xml"

static void item_menu_drawer(int row, const char* title, void* ctx)
{
	TestCase* test_case = (TestCase*)ctx;

	int max_length = gr_fb_width() / CHAR_WIDTH - 2;
	char format_buf[20];
	char item_buf[max_length];

	int passed = test_case_get_status(test_case);
	sprintf(format_buf, "%s%%%d%c", "%s", max_length - strlen(title), 'c');

	//LOGE("%s: format_buf-- %s", __func__, format_buf);
	LOGE("%s: passed is %d", __func__,  passed);

	if (passed == 0) {
		sprintf(item_buf, format_buf, title, 'F');
		gr_color(255, 0, 0, 255);
	} else if (passed == 1) {
		sprintf(item_buf, format_buf, title, 'P');
		gr_color(0, 255, 0, 255);
	} else {  // passed == -1
		sprintf(item_buf, format_buf, title, '-');
	//	gr_color(64, 96, 255, 255);
	}

	gr_text(0, (row+1)*CHAR_HEIGHT-1, item_buf);
}

void item_menu_handler(void* ctx)
{
	TestCase* test_case = (TestCase*)ctx;

	test_case_run(test_case);
}

void full_test_handler(void* ctx)
{
	LOGE("in full test handler");
	size_t count = 0;
	size_t i = 0;
	TestCaseManager* cases_manager = (TestCaseManager*)ctx;

	count = test_case_manager_case_count(cases_manager);

	for (i = 0; i < count; i++) {
		test_case_run(test_case_manager_get_by_id(cases_manager, i));
	}
}

void item_test_handler(void* ctx)
{
	LOGE("in item test handler");
	Menu* items_menu = (Menu*)ctx;

	menu_draw(items_menu);
}

void view_test_result_handler(void* ctx)
{
	// TODO support multi page report
	LOGE("in view_test_result_handler");
	size_t count = 0;
	size_t i = 0;
	int start_x = 0;
	int start_y = 1;
	char format_buf[20];
	int max_line_char = gr_fb_width()/CHAR_WIDTH - 2;
	char line_buf[max_line_char];

	TestCaseManager* cases_manager = (TestCaseManager*)ctx;	

	ui_screen_clean();
	count = test_case_manager_case_count(cases_manager);
	for (i = 0; i < count; i++) {
		char* case_name = NULL;
		TestCase* test_case = test_case_manager_get_by_id(cases_manager, i);
		if (test_case == NULL) {
			break;
		}
		test_case_get_name(test_case, (const char**)&case_name);
		int passed = test_case_get_status(test_case);
		memset(line_buf, 0, max_line_char);
		memset(format_buf, 0, 20);
		sprintf(format_buf, "%s%%%d%c", "%s", max_line_char - strlen(case_name) - 4, 's');

		if (passed == 0) {
			sprintf(line_buf, format_buf, case_name, "Fail");
			gr_color(255, 0, 0, 255);
		} else if (passed == 1) {
			sprintf(line_buf, format_buf, case_name, "Pass");
			gr_color(0, 255, 0, 255);
		} else {  // passed == -1
			sprintf(line_buf, format_buf, case_name, "----");
			gr_color(64, 96, 255, 255);
		}

		gr_text(0, start_y*CHAR_HEIGHT, line_buf);
		start_y++;
	}
	gr_flip();

	ui_wait_anykey();
}

void exit_handler(void* ctx)
{
	LOGE("in exit handler");
	Menu* menu = (Menu*)ctx;

	menu_quit(menu);
}

int main(int argc, char* argv[])
{
	int fd, rc;
	char cmdline[FK_CMD_LEN];

	//fk_sqlite_create();
	fd = open("/proc/cmdline", O_RDONLY);
	if (fd < 0)
		return -1;

	memset(cmdline, 0, sizeof(cmdline));
	rc = read(fd, cmdline, sizeof(cmdline));
	LOGE("FK_MODE: cmdline=%s\n", cmdline);

	if (rc > 0) {
		if (strstr(cmdline, ENG_TEST) == NULL) return -1;
	} else {
		return -1;
	}

	system("rmmod unifi_sdio");
	system("insmod /system/lib/modules/unifi_sdio.ko &");

	ui_init();

	fk_sqlite_create();

	FkConfig* config = fk_config_expat_create();

	fk_config_load(config, XML_CONFIG_FILE);
	fk_set_config(config);

	TestCaseManager* cases_manager = test_case_manager_create(config);
	fk_set_test_case_manager(cases_manager);

	Menu* item_menu = menu_create(15);
	menu_add_title(item_menu, "Topwise Item Test");

	// add menu item 
	int i = 0;
	int count = 0;
	count = test_case_manager_case_count(cases_manager);
	for (i = 0; i < count; i++) {
		char* case_name = NULL;
		TestCase* testcase = test_case_manager_get_by_id(cases_manager, i);
		if (testcase != NULL) {
			test_case_get_name(testcase, (const char**)&case_name);
			menu_add_item(item_menu, case_name, item_menu_handler, (void*)testcase, item_menu_drawer);
		}
	}

	menu_add_item(item_menu, "Back", exit_handler, (void*)item_menu, NULL);

	Menu* menu = menu_create(5);

	MenuItem main_menu[4] = {
		{"Full Phone Test", full_test_handler, (void*)cases_manager, NULL},
		{"Item Test", item_test_handler, (void*)item_menu, NULL},
		{"View Test Result", view_test_result_handler, (void*)cases_manager, NULL},
		{"Exit", exit_handler, (void*)menu, NULL},
	};

	menu_add_title(menu, "Topwise company");
	menu_add_title(menu, "factroy kit");
	//menu_add_title(menu, "by zxsu");

	for (i = 0; i < 4; i++) {
		menu_add_item(menu, main_menu[i].item, main_menu[i].handler, main_menu[i].item_ctx, main_menu[i].drawer);
	}

	menu_draw(menu);

	menu_destroy(menu);
	menu_destroy(item_menu);

	LOGE("%s: destroy case_manager", __func__);
	test_case_manager_destroy(cases_manager);
	fk_config_destroy(config);

	LOGE("exit factory kit");
	// reboot system
	sync();
	reboot(RB_AUTOBOOT);

	return 0;
}
