#include "keypress_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "utils.h"

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define HWINFO_PATH "/proc/hwinfo"
#define KEY_MAP_S "key_map"
#define KEY_INPUT "sprd-keypad"
#define WAKE_MSG "wake"

#define MAX_KEYS 20

typedef struct {
	CmdListener* listener;

	int keys[MAX_KEYS];
	int keys_count;

	int wait_keys;
	int ctrl_fd[2];
} PrivInfo;

static int hexstr2dec(const char* hex)
{
	int dec = 0;
	int size = strlen(hex);
	int i;

	for (i = 0; i < size; i++) {
		int m = 0;
		char c = hex[size - i - 1];
		if ( c == 'X' || c == 'x' )
			break;
		if (c >= '0' && c <= '9') {
			m = c - '0';
		} else if (c >= 'A' && c <= 'F') {
			m = c - 'A' + 10;
		} else if (c >= 'a' && c <= 'f') {
			m = c - 'a' + 10;
		}

		if (i == 0) {
			dec += m;
		} else {
			dec +=  (int)pow(16,i) * m;
		}
	}

	return dec;
}
	                    
static void* keypress_cmd_timer_thread(void* ctx)
{
	CmdInterface* thiz = (CmdInterface*)ctx;
	DECLES_PRIV(priv, thiz);
	int i = 0;

	// wait 
	while(priv->wait_keys == 1 && i < 30) {
		usleep(200000);
		i++;
	}

	priv->wait_keys = 0;
	LOGE("%s: i = %d send WAKE_MSG %s", __func__, i, WAKE_MSG);
	write(priv->ctrl_fd[1], WAKE_MSG, strlen(WAKE_MSG));

	return NULL;
}

static void keypress_cmd_init_keys(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);

	FILE* fd;
	char buffer[1024];
	char* s = NULL;
	char* ptr = NULL;
	int i = 0;

	fd = fopen(HWINFO_PATH, "r");

	if (fd == NULL) {
		LOGE("%s: can not open %s", __func__, HWINFO_PATH);
		return;
	}

	do {
		s = fgets(buffer, 1024, fd);
		if (s == NULL) {
			goto out;
		}
		ptr = strstr(s, KEY_MAP_S);
		if (ptr != NULL) {
			LOGE("%s: get keymay: %s", __func__, ptr);
			break;
		}
	} while(1);

	ptr = strchr(ptr, '=');
	ptr++;

	s = strtok(ptr, ",");
	i = 0;
	while(s != NULL) {
		if (i % 2) {
			while(*s != '0') {
				s++;
			}
			ptr = s;
			while(*ptr != '\0') 
				ptr++;
			ptr--;

			while(isspace(*ptr)) {
				*ptr = '\0';
				ptr--;
			}

			priv->keys[priv->keys_count] = hexstr2dec(s);
			priv->keys_count ++;
		}

		s = strtok(NULL, ",");
		i++;
	}

	priv->keys[priv->keys_count] = 0x74;
	priv->keys_count++;
out:
	fclose(fd);
}

static int keypress_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	//keypress_cmd_init_keys(thiz);
	char content[4];
	pthread_t t;
	int ret;
	char buf[128];
	int pass = 0;
	struct pollfd* mfds;
	mfds = (struct pollfd*)malloc(2 * sizeof(struct pollfd));
	memset(mfds, 0, 2 * sizeof(struct pollfd));
	memset(content, 0, sizeof(content));
	int i = 0;

	for (i = 0; i < priv->keys_count; i++) {
		LOGE("%s: keys[%d] = 0x%x", __func__, i, priv->keys[i]);
	}

	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_KEYPRESS);

	// reply AUTOTEST_RET_STEP1
	*(uint8_t*)(content) = AUTOTEST_RET_STEP1;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	int fd = open_input(KEY_INPUT, O_RDONLY);
	if (fd < 0) {
		pass = 0;
		goto out;
	}

	mfds[0].fd = fd;
	mfds[0].events = POLLIN;
	mfds[1].fd = priv->ctrl_fd[0];
	mfds[1].events = POLLIN;

	//start
	priv->wait_keys = 1;
	pthread_create(&t, NULL, keypress_cmd_timer_thread, (void*)thiz);

	while(priv->wait_keys == 1) {
		struct input_event event;
		int key = 0;
		i = 0;
		ret = poll(mfds, 2, -1);
		
		if (ret < 0) {
			break;
		}

		if (mfds[0].revents == POLLIN) {
			LOGE("%s: receive input event", __func__);
			ret = read(fd, &event, sizeof(event));
			if (event.type == EV_SYN) {
				continue;
			} else if (event.type == EV_KEY && event.value > 0) {
				key = event.code;
				LOGE("%s: get keyevent 0x%x, value: %d", __func__, key, event.value);

				for (i = 0; i < priv->keys_count; i++) {
					if (priv->keys[i] == key) {
						priv->keys[i] = 0xffff;
					}
				}
			} else {
				LOGE("%s: get keyevent 0x%x, value: %d", __func__, event.code, event.value);
			}
		}

		if (mfds[1].revents == POLLIN) {
			LOGE("%s: receive wake event", __func__);
			read(mfds[1].fd, buf, sizeof(buf));
			LOGE("%s: get pipe msg: %s", __func__, buf);
		}

		for (i = 0; i < priv->keys_count; i++) {
			if (priv->keys[i] != 0xffff)
				break;
		}

		if (i == priv->keys_count) {
			LOGE("%s: pass the test break", __func__);
			pass = 1;
			break;
		}
	}

	priv->wait_keys = 0;
	pthread_join(t, NULL);

out:
	if (pass) {
		*(uint8_t*)(content) = AUTOTEST_RET_PASS;
	} else {
		*(uint8_t*)(content) = AUTOTEST_RET_FAIL;
	}

	for (i = 0; i < priv->keys_count; i++) {
		LOGE("%s: keys[%d] = 0x%x", __func__, i, priv->keys[i]);
	}

	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	if (fd >= 0) {
		close(fd);
	}
	free(mfds);

	return 0;
}

static void keypress_cmd_destroy(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);

	close(priv->ctrl_fd[0]);
	close(priv->ctrl_fd[1]);

	SAFE_FREE(thiz);
}

CmdInterface* keypress_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = keypress_cmd_execute;
		thiz->destroy = keypress_cmd_destroy;

		thiz->cmd = AUTOTEST_KEYPRESS;
		priv->listener = listener;
		priv->keys_count = 0;
		keypress_cmd_init_keys(thiz);

		pipe(priv->ctrl_fd);
	}

	return thiz;
}
