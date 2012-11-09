#include "backlight_test_case.h"
#include "ui.h"
#include "minui.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define LOG_TAG "backlight"
#include <utils/Log.h>

#define LCD_BACKLIGHT_DEV           "/sys/class/leds/lcd-backlight/brightness"
#define KEY_BACKLIGHT_DEV           "/sys/class/leds/button-backlight/brightness"
#define LCD_BACKLIGHT_MAX_DEV       "/sys/class/leds/lcd-backlight/max_brightness"
#define KEY_BACKLIGHT_MAX_DEV       "/sys/class/leds/button-backlight/max_brightness"

typedef struct {
	int key_fd;
	int lcd_fd;
	int key_max_bright;
	int lcd_max_bright;
} PrivInfo;

static int backlight_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int i = 0;
	int ret = 0;
	char lcd_buf[8];
	char key_buf[8];
	
	memset(lcd_buf, 0, sizeof(lcd_buf));
	memset(key_buf, 0, sizeof(key_buf));

	sprintf(lcd_buf, "%d", priv->lcd_max_bright);
	sprintf(key_buf, "%d", priv->key_max_bright);

	ui_screen_clean();

	gr_color(0, 255, 0, 255);
	ui_draw_title(UI_TITLE_START_Y, BACKLIGHT_TEST_CASE);
	gr_flip();

	for (i = 0; i < 4; i++) {
		write(priv->lcd_fd, lcd_buf, strlen(lcd_buf));
		write(priv->key_fd, key_buf, strlen(key_buf));
		usleep(500000);
		write(priv->lcd_fd, "0", 1);
		write(priv->key_fd, "0", 1);
		usleep(500000);
	}

	write(priv->lcd_fd, lcd_buf, strlen(lcd_buf));
	write(priv->key_fd, key_buf, strlen(key_buf));

	ret = ui_draw_handle_softkey(thiz->name);
	thiz->passed = ret;

	return 0;
}

static void backlight_test_destroy(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	if (priv->key_fd > 0) {
		close(priv->key_fd);
		priv->key_fd = -1;
	}

	if (priv->lcd_fd > 0) {
		close(priv->lcd_fd);
		priv->lcd_fd = -1;
	}

	SAFE_FREE(thiz);
}

TestCase* backlight_test_case_create(int passed)
{
	TestCase* thiz = (TestCase*) malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		int fd;
		char buffer[8];
		DECLES_PRIV(priv, thiz);
		thiz->run = backlight_test_run;
		thiz->destroy = backlight_test_destroy;

		thiz->passed = passed;
		thiz->name = BACKLIGHT_TEST_CASE;

		priv->key_fd = open(KEY_BACKLIGHT_DEV, O_RDWR);
		priv->lcd_fd = open(LCD_BACKLIGHT_DEV, O_RDWR);

		fd = open(KEY_BACKLIGHT_MAX_DEV, O_RDONLY);
		if (fd < 0) {
			LOGE("open %s fail", KEY_BACKLIGHT_MAX_DEV);
			priv->key_max_bright = 0;
		} else {
			memset(buffer, 0, sizeof(buffer));
			read(fd, buffer, sizeof(buffer));
			priv->key_max_bright = atoi(buffer);
			close(fd);
		}

		fd = open(LCD_BACKLIGHT_MAX_DEV, O_RDONLY);
		if (fd < 0) {
			LOGE("open %s fail", LCD_BACKLIGHT_MAX_DEV);
			priv->lcd_max_bright = 0;
		} else {
			memset(buffer, 0, sizeof(buffer));
			read(fd, buffer, sizeof(buffer));
			priv->lcd_max_bright = atoi(buffer);
			close(fd);
		}
	}

	return thiz;
}
