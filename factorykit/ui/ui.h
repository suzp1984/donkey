#ifndef UI_H
#define UI_H

#define MAX_COLS 64
#define MAX_ROWS 32

#define CHAR_WIDTH 10
#define CHAR_HEIGHT 18

#define UI_TITLE_START_Y 30

#define NO_ACTION           -1

#define HIGHLIGHT_UP        -2
#define HIGHLIGHT_DOWN      -3
#define SELECT_ITEM         -4
#define KEY_HOME_GO       -5
    

void ui_init(void);
void ui_clear_key_queue();
int ui_wait_key();
int device_handle_key(int key_code);

void ui_screen_clean(void);
void ui_draw_title(int start_y, const char* t);
void ui_draw_softkey(void);
int ui_draw_handle_softkey(const char* name);
int ui_handle_softkey(const char* name);

void ui_wait_anykey();
void ui_quit_with_prompt(const char* name, int result);
void ui_draw_prompt(const char* prompt);
void ui_draw_prompt_noblock(int result);
void ui_quit_by_result(const char* name, int result);

#endif // UI_H
