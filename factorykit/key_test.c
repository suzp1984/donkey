#include "key_test.h"
#include "ui.h"
#include "minui.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define BUFFER_LENGTH           256
#define KEYNAME_LENGHT          32
#define KEYLAYOUT_FILE_NAME     "sprd-keypad"
#define MAX_KEY_COUNTS  32

#define KEY_TOKEN "key"
#define KEY_TOKEN_B "KEY"

#define KEY_COLUMN_LINE         5
#define KEY_ROW_LINE            7
#define KEY_ITEM_WIDTH 	       75
#define KEY_ITEM_HEIGHT        50
#define KEY_TABLE_X			 10
#define KEY_TABLE_Y 		 40

#define POWER_KEYCODE 116

typedef struct {
	int start_x;
	int start_y;

	int pressed;

	int keycode;
	char keyname[KEYNAME_LENGHT];
} keyinfo_t;

typedef struct {
	int key_counts;
	keyinfo_t key_info[MAX_KEY_COUNTS];
} PrivInfo;

static int key_test_init(TestCase* thiz);
static int key_test_parser(TestCase* thiz, char* line, int length);
static int key_test_show(TestCase* thiz);
static int key_test_key_pressed(TestCase* thiz, int key);

static int key_test_key_pressed(TestCase* thiz, int key)
{
	DECLES_PRIV(priv, thiz);
	int i = 0;

	for (i = 0; i < priv->key_counts; i++) {
		if (priv->key_info[i].keycode == key) {
			priv->key_info[i].pressed = 1;
			return 1;
		}
	}

	return 0;
}

static int key_test_show(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	ui_screen_clean();
	int i = 0;
	int colume_width = KEY_ITEM_WIDTH / CHAR_WIDTH;

	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, KEY_TEST_CASE);

	gr_color(64, 96, 255, 255);

	for (i = 0; i < KEY_COLUMN_LINE; i++) {
		gr_fill(KEY_TABLE_X + i * KEY_ITEM_WIDTH, KEY_TABLE_Y, KEY_TABLE_X + i * KEY_ITEM_WIDTH + 2,
				KEY_TABLE_Y + (KEY_ROW_LINE - 1) * KEY_ITEM_HEIGHT);
	}

	for (i = 0; i < KEY_ROW_LINE; i++) {
		gr_fill(KEY_TABLE_X, KEY_TABLE_Y + i * KEY_ITEM_HEIGHT,
				KEY_TABLE_X + (KEY_COLUMN_LINE - 1) * KEY_ITEM_WIDTH, KEY_TABLE_Y + i * KEY_ITEM_HEIGHT + 2);
	}

	gr_color(255, 255, 0, 255);
	for (i = 0; i < priv->key_counts; i++) {
		if (!priv->key_info[i].pressed) {
			if ((int)strlen(priv->key_info[i].keyname) > colume_width) {
				char name[colume_width];
				memcpy(name, priv->key_info[i].keyname, colume_width);
				gr_text(priv->key_info[i].start_x, priv->key_info[i].start_y, name);
				gr_text(priv->key_info[i].start_x, priv->key_info[i].start_y + CHAR_HEIGHT, 
						priv->key_info[i].keyname + colume_width);
			} else {

				gr_text(priv->key_info[i].start_x, priv->key_info[i].start_y, priv->key_info[i].keyname);
			}
		}
	}

	gr_flip();

	return 0;
}

static int key_test_parser(TestCase* thiz, char* line, int length)
{
	DECLES_PRIV(priv, thiz);
	char* s = line;
	char* w = NULL;

	while (isspace(*s)) {
		s++;
	}

	if (*s == '#' || s == NULL) {
		return -1;
	}

	w = strtok(s, " ");
	if (w == NULL) {
		return 0;
	}

	if (!strncmp(w, KEY_TOKEN, strlen(KEY_TOKEN)) ||
			!strncmp(w, KEY_TOKEN_B, strlen(KEY_TOKEN_B))) {
		w = strtok(NULL, " ");
		priv->key_info[priv->key_counts].keycode = atoi(w);
		w = strtok(NULL, " ");

		if (w[strlen(w) - 1] == '\n') {
			w[strlen(w) - 1] = '\0';
		}
		memcpy(priv->key_info[priv->key_counts].keyname, w, strlen(w));
		priv->key_counts++;
	}

	return 0;
}

static int key_test_init(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	priv->key_counts = 0;
	memset(priv->key_info, 0, sizeof(priv->key_info));

	int i;
	FILE* fp;
	char buffer[BUFFER_LENGTH];
	char keylayout_name[300];
	char oemkeylayout[300];
	struct stat fstat;

	const char* root = getenv("ANDROID_ROOT");

	memset(oemkeylayout, 0, sizeof(oemkeylayout));
	snprintf(oemkeylayout, sizeof(oemkeylayout),
			"%s/oem/%s.kl", root, KEYLAYOUT_FILE_NAME);

	memset(keylayout_name, 0, sizeof(keylayout_name));
	snprintf(keylayout_name, sizeof(keylayout_name),
			"%s/usr/keylayout/%s.kl", root, KEYLAYOUT_FILE_NAME);

	if (stat(oemkeylayout, &fstat)) {
		fp = fopen(keylayout_name, "rb");
	} else {
		fp = fopen(oemkeylayout, "rb");
	}
	//LOGE("%s: keylayout_name is [%s]", __func__, keylayout_name);

	while(!feof(fp)) {
		memset(buffer, 0, sizeof(buffer));
		fgets(buffer, BUFFER_LENGTH, fp);
		// parser a line
		key_test_parser(thiz, buffer, BUFFER_LENGTH);
	}

	fclose(fp);

	// key layout setup x,y 
	for (i = 0; i < priv->key_counts; i++) {
		int m = i % (KEY_COLUMN_LINE - 1);
		int n = i / (KEY_COLUMN_LINE - 1);
		LOGE("%s: key %s, colume (%d), line (%d)", __func__, priv->key_info[i].keyname, m, n);
		priv->key_info[i].start_x = KEY_TABLE_X + m * KEY_ITEM_WIDTH + 2;
		priv->key_info[i].start_y = KEY_TABLE_Y + n * KEY_ITEM_HEIGHT + 2 + CHAR_HEIGHT;
	}

	return 0;
}

static int key_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int i = 0;
	int key = 0;
	int key_counts = 0;
	int power_counts = 0;

	ui_screen_clean();
	thiz->passed = 0;
	key_test_init(thiz);

	for (i = 0; i < priv->key_counts; i++) {
		LOGE("%s: %s = %d", __func__, priv->key_info[i].keyname, priv->key_info[i].keycode);
	}

	key_test_show(thiz);

	while(1) {
		key_counts = 0;
		for (i = 0; i < priv->key_counts; i++) {
			if (priv->key_info[i].pressed == 1) {
				key_counts ++;
			}
		}

		if (key_counts == priv->key_counts) {
			thiz->passed = 1;
			break;
		}

		key = ui_wait_key();
		if (key_test_key_pressed(thiz, key)) {
			key_test_show(thiz);
		}

		if(key == POWER_KEYCODE) {
			power_counts++;
			if (power_counts > 3) {
				break;
			}
		}
	}

	ui_quit_with_prompt(KEY_TEST_CASE, thiz->passed);

	return 0;
}

static void key_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* key_test_create(int passed)
{
	TestCase* thiz = (TestCase*) malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		thiz->run = key_test_run;
		thiz->destroy = key_test_destroy;

		thiz->name = KEY_TEST_CASE;
		thiz->passed = passed;
	}

	return thiz;
}
