#include "bluetooth_test.h"
#include "ui.h"
#include "minui.h"

#include <stdlib.h>
#include <sys/types.h>       
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define BT_PROMPT "Open BT device..."

static int create_hci_socket(void);

static int create_hci_socket(void) 
{
	int sk = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (sk < 0) {
		LOGE("%s: Failed to create bluetooth hci socket: %s (%d)",
				__func__, strerror(errno), errno);
	}

	return sk;
}

static int bluetooth_test_run(TestCase* thiz)
{
	int hci_sock;
	int attempt;
	ui_screen_clean();

	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, BLUETOOTH_TEST_CASE);

	gr_color(64, 96, 255, 255);
	ui_draw_title(gr_fb_height()/2, BT_PROMPT);
	gr_flip();

	system("hciconfig hci0 down");
	usleep(10000);

	for (attempt = 500; attempt > 0; attempt--) {
		hci_sock = create_hci_socket();
		if (hci_sock < 0) {
			attempt = 0;
			break;
		}

		if (!ioctl(hci_sock, HCIDEVUP, 0)) {
			break;
		}

		close(hci_sock);
		usleep(10000);
	}

	gr_color(0, 0, 0, 255);
	ui_draw_title(gr_fb_height()/2, BT_PROMPT);

	system("hciconfig hci0 down");
	usleep(10000);

	thiz->passed = (attempt == 0) ? 0 : 1;
	ui_quit_with_prompt(thiz->name, thiz->passed);
	return 0;
}

static void bluetooth_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* bluetooth_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase));

	if (thiz != NULL) {
		thiz->run = bluetooth_test_run;
		thiz->destroy = bluetooth_test_destroy;

		thiz->name = BLUETOOTH_TEST_CASE;
		thiz->passed = passed;
	}

	return thiz;
}
