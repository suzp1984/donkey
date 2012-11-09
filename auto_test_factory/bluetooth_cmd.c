#include "bluetooth_cmd.h"
#include "cmd_common.h"
#include "parcel.h"

#include <stdlib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <unistd.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

typedef struct {
	CmdListener* listener;
} PrivInfo;

static int bluetooth_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	int hci_sock;
	int attempt;
	char content[4];

	system("hciconfig hci0 down");

	for(attempt = 500; attempt > 0; attempt--) {
		hci_sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
		if (hci_sock < 0) {
			LOGE("%s: fail to create Bluetooth HCI socket", __func__);
			attempt = 0;
			break;
		}

		if (!ioctl(hci_sock, HCIDEVUP, 0)) {
			break;
		}

		close(hci_sock);
		usleep(10000);
	}

	system("hciconfig hci0 down");

	memset(content, 0, sizeof(content));
	if (attempt > 0) {
		*(uint8_t*)content = AUTOTEST_RET_PASS;
	} else {
		*(uint8_t*)content = AUTOTEST_RET_FAIL;
	}

	Parcel* reply = parcel_create();
	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_BLUETOOTH);
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);
	return 0;
}

static void bluetooth_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* bluetooth_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*)malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = bluetooth_cmd_execute;
		thiz->destroy = bluetooth_cmd_destroy;

		thiz->cmd = AUTOTEST_BLUETOOTH;
		priv->listener = listener;
	}

	return thiz;
}
