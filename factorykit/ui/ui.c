/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <cutils/properties.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#include "common.h"
#include "minui.h"
#include "ui.h"
#include "fk_sqlite.h"

#define SOFTKEY_OK                    "PASS: HOME"
#define SOFTKEY_FAIL                  "FAIL: POWER"
#define PASS_TEST_MSG "PASS"
#define FAIL_TEST_MSG "FAIL"
#define WAIT_PROMPT_MSG "press anykey to continue"

// Key event input queue
static pthread_mutex_t key_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_queue_cond = PTHREAD_COND_INITIALIZER;
static int key_queue[256], key_queue_len = 0;
static volatile char key_pressed[KEY_MAX + 1];

int key_up, key_down, key_home, key_enter;

// Reads input events, handles special hot keys, and adds to the key queue.
static void *input_thread(void *cookie)
{
    int rel_sum = 0;
    int fake_key = 0;
    for (;;) {
        // wait for the next key event
        struct input_event ev;
        do {
            ev_get(&ev, 0);

            if (ev.type == EV_SYN) {
                continue;
            } else if (ev.type == EV_REL) {
                if (ev.code == REL_Y) {
                    // accumulate the up or down motion reported by
                    // the trackball.  When it exceeds a threshold
                    // (positive or negative), fake an up/down
                    // key event.
                    rel_sum += ev.value;
                    if (rel_sum > 3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_DOWN;
                        ev.value = 1;
                        rel_sum = 0;
                    } else if (rel_sum < -3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_UP;
                        ev.value = 1;
                        rel_sum = 0;
                    }
                }
            } else {
                rel_sum = 0;
            }
        } while (ev.type != EV_KEY || ev.code > KEY_MAX);
        pthread_mutex_lock(&key_queue_mutex);
        if (!fake_key) {
            // our "fake" keys only report a key-down event (no
            // key-up), so don't record them in the key_pressed
            // table.
            key_pressed[ev.code] = ev.value;
        }
        fake_key = 0;
        const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
        if (ev.value > 0 && key_queue_len < queue_max) {
            key_queue[key_queue_len++] = ev.code;
            pthread_cond_signal(&key_queue_cond);
        }
        pthread_mutex_unlock(&key_queue_mutex);

    }
    return NULL;
}

void ui_init(void)
{
	char buf[8];
    gr_init();
    ev_init();

    pthread_t t;
    pthread_create(&t, NULL, input_thread, NULL);

    // key init
	memset(buf,0,sizeof(buf));
	property_get("ro.recvkey.up", buf, "");
	key_up = atoi(buf);

	memset(buf,0,sizeof(buf));
	property_get("ro.recvkey.down", buf, "");
	key_down = atoi(buf);

	memset(buf,0,sizeof(buf));
	property_get("ro.recvkey.enter", buf, "");
	key_enter = atoi(buf);

	memset(buf,0,sizeof(buf));
	property_get("ro.recvkey.home", buf, "");
	key_home = atoi(buf);
	
	//fk_sqlite_create();
}

void ui_clear_key_queue() {
	pthread_mutex_lock(&key_queue_mutex);
	key_queue_len = 0;
	pthread_mutex_unlock(&key_queue_mutex);
}  

int ui_wait_key()
{       
	pthread_mutex_lock(&key_queue_mutex);
	while (key_queue_len == 0) {
		pthread_cond_wait(&key_queue_cond, &key_queue_mutex);
	}

	int key = key_queue[0];
	memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
	pthread_mutex_unlock(&key_queue_mutex);
	return key;
}

int device_handle_key(int key_code) {
	if(key_code == key_down)
		return HIGHLIGHT_DOWN;

	if(key_code == key_up)
		return HIGHLIGHT_UP; 

	if(key_code == key_enter)
		return SELECT_ITEM;

	if (key_code == key_home)
		return KEY_HOME_GO;

	return NO_ACTION;
}

void ui_screen_clean(void)
{
	gr_color(0, 0, 0, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());
}

void ui_draw_title(int start_y, const char* t)
{
	int width;
	int start_x;
	int length = strlen(t);

	if (t[0] != '\0') {
		width = gr_fb_width();
		start_x = (width - length*CHAR_WIDTH)/2;
		gr_text(start_x, start_y, t);
	}
}

void ui_draw_softkey(void)
{
	int width = gr_fb_width();
	int height = gr_fb_height();

	gr_color(0, 255, 0, 255);
	gr_text(10, height - CHAR_HEIGHT, SOFTKEY_OK);

	gr_color(255, 0, 0, 255);
	gr_text(width - CHAR_WIDTH*strlen(SOFTKEY_FAIL), height - CHAR_HEIGHT, SOFTKEY_FAIL);
}

int ui_draw_handle_softkey(const char* name)
{
	int width = gr_fb_width();
	int height = gr_fb_height();

	gr_color(0, 255, 0, 255);
	gr_text(10, height - CHAR_HEIGHT, SOFTKEY_OK);

	gr_color(255, 0, 0, 255);
	gr_text(width - CHAR_WIDTH*strlen(SOFTKEY_FAIL), height - CHAR_HEIGHT, SOFTKEY_FAIL);

	gr_flip();
	
	return ui_handle_softkey(name);
}

int ui_handle_softkey(const char* name)
{
	ui_clear_key_queue();
	for (;;) {
		int key = ui_wait_key();
		int action = device_handle_key(key);
		if (action < 0) {
			switch(action) {
				case SELECT_ITEM:
					// fail;
					//LOGE("%s: set test case FAIL", __func__);
					fk_sql_str2int_set(name, 0);
					return 0;
				case KEY_HOME_GO:
					// pass;
					//LOGE("%s: set test case PASS", __func__);
					fk_sql_str2int_set(name, 1);
					return 1;
				case NO_ACTION:
					break;
			}
		}
	}
}

void ui_wait_anykey()
{
	ui_clear_key_queue();
	ui_wait_key();
}
			
void ui_quit_with_prompt(const char* name, int result)
{
	int width = gr_fb_width();
	int height = gr_fb_height();

	if (result > 0) {
		gr_color(0, 255, 0, 255);
		gr_text((width - CHAR_WIDTH * strlen(PASS_TEST_MSG)) / 2, 
				height - 2 * CHAR_HEIGHT, PASS_TEST_MSG);
	} else {
		gr_color(255, 0, 0, 255);
		gr_text((width - CHAR_WIDTH * strlen(FAIL_TEST_MSG)) / 2,
				height - 2 * CHAR_HEIGHT, FAIL_TEST_MSG);
	}

	gr_text ((width - CHAR_WIDTH * strlen(WAIT_PROMPT_MSG)) / 2, 
			height - CHAR_HEIGHT, WAIT_PROMPT_MSG);

	gr_flip();

	ui_wait_anykey();
	fk_sql_str2int_set(name, result > 0 ? 1 : 0);
}

void ui_draw_prompt(const char* prompt)
{
	int width = gr_fb_width();
	int height = gr_fb_height();

	gr_text((width - CHAR_WIDTH * strlen(prompt)) / 2,
			height - 2 * CHAR_HEIGHT, prompt);

	gr_flip();
	ui_wait_anykey();
}

void ui_draw_prompt_noblock(int result)
{
	int width = gr_fb_width();
	int height = gr_fb_height();

	if (result > 0) {
		gr_color(0, 0, 0, 255);
		gr_text((width - CHAR_WIDTH * strlen(FAIL_TEST_MSG)) / 2,
				height - 2 * CHAR_HEIGHT, FAIL_TEST_MSG);

		gr_color(0, 255, 0, 255);
		gr_text((width - CHAR_WIDTH * strlen(PASS_TEST_MSG)) / 2, 
				height - 2 * CHAR_HEIGHT, PASS_TEST_MSG);
	} else {
		gr_color(0, 0, 0, 255);
		gr_text((width - CHAR_WIDTH * strlen(PASS_TEST_MSG)) / 2, 
				height - 2 * CHAR_HEIGHT, PASS_TEST_MSG);

		gr_color(255, 0, 0, 255);
		gr_text((width - CHAR_WIDTH * strlen(FAIL_TEST_MSG)) / 2,
				height - 2 * CHAR_HEIGHT, FAIL_TEST_MSG);
	}

	gr_text ((width - CHAR_WIDTH * strlen(WAIT_PROMPT_MSG)) / 2, 
			height - CHAR_HEIGHT, WAIT_PROMPT_MSG);

	gr_flip();

}

void ui_quit_by_result(const char* name, int result)
{
	fk_sql_str2int_set(name, result > 0 ? 1 : 0);
}
