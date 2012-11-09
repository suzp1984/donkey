#ifndef MENU_FK_H
#define MENU_FK_H

#include "typedef.h"

struct _Menu;
typedef struct _Menu Menu;

typedef void (*MenuHandler)(void* ctx);
typedef void (*MenuItemDrawer)(int row, const char* item, void* ctx);

typedef struct {
	char* item;
	MenuHandler handler;
	void* item_ctx;
	MenuItemDrawer drawer;
} MenuItem;

Menu* menu_create(size_t count);

Ret menu_add_title(Menu* thiz, const char* title);
Ret menu_add_item(Menu* thiz, const char* item, MenuHandler handler, void* item_ctx, MenuItemDrawer drawer);
Ret menu_del_item(Menu* thiz, const char* item);
Ret menu_draw(Menu* thiz);
Ret menu_select(Menu* thiz, size_t i);
Ret menu_quit(Menu* thiz);

void menu_destroy(Menu* thiz);

#endif // MENU_FK_H
