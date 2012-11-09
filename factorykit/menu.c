#include "menu.h"
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#include "ui.h"
#include "minui.h"
#include "common.h"

#define DEFAULT_ITEM_COUNT 10


struct _Menu {
	char** title;
	size_t size;
	size_t count;
	int menu_sel;
	int in_loop;

	size_t max_rows;
	size_t max_cols;
	size_t top_menu;
	size_t title_rows;
	MenuItem* items;
};

static void menu_draw_text_line(int row, const char* t, void* ctx);

static void menu_draw_text_line(int row, const char* t, void* ctx) 
{
	if (t[0] != '\0') {
		gr_text(0, (row+1)*CHAR_HEIGHT-1, t);
		gr_color(64, 96, 255, 255);
	}
}

static int menu_update_screen(Menu* thiz)
{
	size_t i = 0;
	int visible_items = 0;

	char** p = thiz->title;
	
	// clean screen
	gr_color(0, 0, 0, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());

	// draw title
	gr_color(64, 96, 255, 255);
	while (*p != NULL) {
		menu_draw_text_line((int)i, *p, NULL);
		i++;
		p++;
	}
	i++;

	// draw top line
	gr_fill(0, thiz->title_rows * CHAR_HEIGHT - CHAR_HEIGHT / 2 - 1,
			gr_fb_width(), thiz->title_rows * CHAR_HEIGHT - CHAR_HEIGHT / 2 + 1);

	if (thiz->title_rows + thiz->count + 1 > thiz->max_rows) {
		visible_items = thiz->max_rows - thiz->title_rows - 1;
	} else {
		visible_items = thiz->count;
	}

	gr_fill(0, (thiz->title_rows + thiz->menu_sel - thiz->top_menu) * CHAR_HEIGHT,
			gr_fb_width(), (thiz->title_rows + thiz->menu_sel - thiz->top_menu + 1) * CHAR_HEIGHT + 1);

	for (i = thiz->title_rows; i <  thiz->title_rows + visible_items; i++) {
		if (i == thiz->title_rows + thiz->menu_sel - thiz->top_menu) {
			gr_color(255, 255, 255, 255);
		} else {
			gr_color(64, 96, 255, 255);
		}
		thiz->items[i - thiz->title_rows + thiz->top_menu].drawer(i, thiz->items[i -
					thiz->title_rows + thiz->top_menu].item, thiz->items[i - 
					thiz->title_rows + thiz->top_menu].item_ctx);
	}

	gr_color(64, 96, 255, 255);
	// draw bottom line
	gr_fill(0, i * CHAR_HEIGHT + CHAR_HEIGHT / 2 - 1,
			gr_fb_width(), i * CHAR_HEIGHT + CHAR_HEIGHT / 2 + 1);

	gr_flip();

	return 0;
}

static int menu_ui_select_up(Menu* thiz)
{
	thiz->menu_sel--;
	if (thiz->menu_sel < 0) {
		thiz->menu_sel = thiz->count-1;
		if (thiz->title_rows + thiz->count + 1 > thiz->max_rows) {
			thiz->top_menu = thiz->count + thiz->title_rows - thiz->max_rows + 1;
		}
	} else if (thiz->menu_sel >= (int)thiz->count) {
		thiz->menu_sel = thiz->count-1;
		if (thiz->title_rows + thiz->count + 1 > thiz->max_rows) {
			thiz->top_menu = 0;
		}
	} 

	if ((int)thiz->top_menu > thiz->menu_sel) {
		thiz->top_menu--;
	}
	
	menu_update_screen(thiz);
	return thiz->menu_sel;
}

static int menu_ui_select_down(Menu* thiz)
{
	thiz->menu_sel++;

	if (thiz->menu_sel < 0) {
		thiz->menu_sel = 0;
		if (thiz->top_menu != 0) {
			thiz->top_menu = 0;
		}
	} else if (thiz->menu_sel >= (int)thiz->count) {
		thiz->menu_sel = 0;
		if (thiz->top_menu != 0) {
			thiz->top_menu = 0;
		}
	} 

	if ((int)(thiz->top_menu + thiz->max_rows - thiz->title_rows - 2) < thiz->menu_sel) {
		thiz->top_menu++;
	}

	menu_update_screen(thiz);
	return thiz->menu_sel;
}

static int menu_get_selection(Menu* thiz)
{
	ui_clear_key_queue();
	menu_update_screen(thiz);

	int chosen_item = -1;
	while (chosen_item < 0) {
		int key = ui_wait_key();
		int action = device_handle_key(key);

		if (action < 0) {
			switch (action) {
				case HIGHLIGHT_UP:
					menu_ui_select_up(thiz);
					break;
				case HIGHLIGHT_DOWN:
					//LOGE("**HIGHLIGHT_DOWN");
					menu_ui_select_down(thiz);
					break;
				case SELECT_ITEM:
					chosen_item = thiz->menu_sel;
					break;
				case NO_ACTION:
					break;
			}
		}
	}

	return chosen_item;
}

Ret menu_quit(Menu* thiz)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	thiz->in_loop = 0;

	return RET_OK;
}

Menu* menu_create(size_t count)
{
	Menu* thiz = (Menu*)malloc(sizeof(Menu));

	if (thiz != NULL) {
		thiz->count = 0;
		thiz->in_loop = 0;
		thiz->title = NULL;
		if (count < DEFAULT_ITEM_COUNT) {
			thiz->size = DEFAULT_ITEM_COUNT;
		} else {
			thiz->size = count;
		}

		thiz->items = (MenuItem*)malloc(sizeof(MenuItem) * thiz->size);

		thiz->max_rows = gr_fb_height() / CHAR_HEIGHT;
		thiz->max_cols = gr_fb_width() / CHAR_WIDTH;
		thiz->menu_sel = 0;
		thiz->top_menu = 0;
		thiz->title_rows = 1;
	}

	return thiz;
}

Ret menu_add_title(Menu* thiz, const char* title)
{
	return_val_if_fail(thiz != NULL && title != NULL, RET_INVALID_PARAMS);

	thiz->title_rows++;

	if (thiz->title == NULL) {
		thiz->title = (char**) malloc(sizeof(char*) * 2);
		thiz->title[0] = strdup(title);
		thiz->title[1] = NULL;
		return RET_OK;
	}

	char** p = thiz->title;
	int i = 0;
	while (*p != NULL) {
		p++;
		i++;
	}

	thiz->title = (char**)realloc(thiz->title, sizeof(char*) * (i + 2));

	thiz->title[i] = strdup(title);
	thiz->title[i+1] = NULL;

	return RET_OK;
}

Ret menu_add_item(Menu* thiz, const char* item, MenuHandler handler, void* item_ctx, MenuItemDrawer drawer)
{
	return_val_if_fail(thiz != NULL && item != NULL,
			RET_INVALID_PARAMS);

	if (thiz->count >= thiz->size) {
		thiz->items = (MenuItem*)realloc(thiz->items, (thiz->size + DEFAULT_ITEM_COUNT) * sizeof(MenuItem));

		if (thiz->items != NULL) {
			thiz->size += DEFAULT_ITEM_COUNT;
		} else {
			return RET_OOM;
		}
	}

	thiz->items[thiz->count].item = strdup(item);
	thiz->items[thiz->count].handler = handler;
	thiz->items[thiz->count].item_ctx = item_ctx;
	if (drawer != NULL) {
		thiz->items[thiz->count].drawer = drawer;
	} else {
		thiz->items[thiz->count].drawer = menu_draw_text_line;
	}

	thiz->count++;
	return RET_OK;
}

Ret menu_del_item(Menu* thiz, const char* item) 
{
	return_val_if_fail(thiz != NULL && item != NULL, RET_INVALID_PARAMS);

	int i = 0;
	int j = 0;
	for (i = 0; i < (int)thiz->count; i++) {
		if(!strcmp(item, thiz->items[i].item)) {
			thiz->count--;
			free(thiz->items[i].item);
			for (j = i; j < (int)thiz->count; j++) {
				thiz->items[j].item = thiz->items[j+1].item;
				thiz->items[j].handler = thiz->items[j+1].handler;
				thiz->items[j].item_ctx = thiz->items[j+1].item_ctx;
			}

			thiz->items[thiz->count].item = NULL;
			thiz->items[thiz->count].handler = NULL;
			thiz->items[thiz->count].item_ctx = NULL;

			break;
		}
	}

	return RET_OK;
}

Ret menu_draw(Menu* thiz)
{
	return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

	thiz->in_loop = 1;
	thiz->menu_sel = 0;
	int chosen_item = 0;
	while(thiz->in_loop) {
		chosen_item = menu_get_selection(thiz);
		if (thiz->items[chosen_item].handler != NULL) {
			thiz->items[chosen_item].handler(thiz->items[chosen_item].item_ctx);
		}
	}

	return RET_OK;
}

Ret menu_select(Menu* thiz, size_t i)
{
	return_val_if_fail(thiz != NULL && i < thiz->count, RET_INVALID_PARAMS);

	thiz->items[i].handler(thiz->items[i].item_ctx);

	return RET_OK;
}

void menu_destroy(Menu* thiz)
{
	return_if_fail(thiz != NULL);
	
	LOGE("%s: menu destroy", __func__);
	int i = 0;
	for (i = 0; i < (int)thiz->count; i++) {
		SAFE_FREE(thiz->items[i].item);
	}

	for (i = 0; thiz->title[i] != NULL; i++) {
		SAFE_FREE(thiz->title[i]);
	}

	SAFE_FREE(thiz->title);
	SAFE_FREE(thiz->items);
	SAFE_FREE(thiz);

	return;
}

#ifdef MENU_TEST

#include <stdio.h>

#define HELLO_MSG "hello world"
#define WEATHER_MSG "raining heavly"

void hello_print(void* ctx)
{
	char* msg = (char*)ctx;

	printf("[msg]: %s\n", msg);
}

int main(int argc,  char* argv[])
{
	Menu* menu = menu_create(5);

	menu_add_title(menu, "Topwise company");
	menu_add_title(menu, "Hello");

	menu_add_item(menu, "hello", hello_print, (void*)HELLO_MSG, NULL);
	menu_add_item(menu, "weather", hello_print, (void*)WEATHER_MSG, NULL);
	
	menu_select(menu, 0);
	menu_select(menu, 1);

	menu_destroy(menu);

	return 0;
}

#endif // MENU_TEST


