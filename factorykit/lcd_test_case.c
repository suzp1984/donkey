#include "lcd_test_case.h"
#include "ui.h"
#include "minui.h"

#include <stdlib.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define LCD_PROMPT "Press any key to continue"

static void lcd_test_draw_screen(int r, int g,  int b, int a)
{
	gr_color(r, g, b, a);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());
	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, LCD_TEST_CASE);
	ui_draw_title(gr_fb_height() - 50,  LCD_PROMPT);
	gr_flip();

	ui_wait_anykey();
}

static int lcd_test_run(TestCase* thiz)
{
	int r;
	int g;
	int b;
	int a;
	int res;

	ui_screen_clean();

	r = 0;
	g = 0;
	b = 0;
	a = 255;
	//black;
	lcd_test_draw_screen(r, g, b, a);
	
	// white;
	r = 255;
	g = 255;
	b = 255;
	a = 255;
	lcd_test_draw_screen(r, g, b, a);
	 
	//red;
	r = 255;
	g = 0;
	b = 0;
	a = 255;
	lcd_test_draw_screen(r, g, b, a);

	//green
	r = 0;
	g = 255;
	b = 0;
	a = 255;
	lcd_test_draw_screen(r, g, b, a);

	// blue
	r = 0;
	g = 0;
	b = 255;
	a = 255;
	lcd_test_draw_screen(r, g, b, a);

	ui_screen_clean();

	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, LCD_TEST_CASE);

	res = ui_draw_handle_softkey(thiz->name);

	thiz->passed = res;
	return 0;
}

static void lcd_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* lcd_test_case_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase));

	if (thiz != NULL) {
		thiz->run = lcd_test_run;
		thiz->destroy = lcd_test_destroy;

		thiz->name = LCD_TEST_CASE;
		thiz->passed = passed;
	}

	return thiz;
}
