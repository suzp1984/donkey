#include "cmd_listener.h"
#include "cmd_interface.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <termios.h>
#include <unistd.h>

#include "parcel.h"
#include "cmd_common.h"
#include "queue.h"

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#include "version_info_cmd.h"
#include "getoption_cmd.h"
#include "backlight_cmd.h"
#include "tp_cmd.h"
#include "vibrator_cmd.h"
#include "echoloop_cmd.h"
#include "echoloop2_cmd.h"
#include "keypress_cmd.h"
#include "receive_cmd.h"
#include "lcd_cmd.h"
#include "ring_cmd.h"
#include "headset_cmd.h"
#include "cft_cmd.h"
#include "flashlight_cmd.h"
#include "led_cmd.h"
#include "pl_sensor_cmd.h"
#include "charge_cmd.h"
#include "fm_cmd.h"
#include "memorycard_cmd.h"
#include "rtc_cmd.h"
#include "camera_cmd.h"
#include "camera1_cmd.h"
#include "gsensor_cmd.h"
#include "sim_cmd.h"
#include "bluetooth_cmd.h"
#include "wifi_cmd.h"
#include "over_cmd.h"
#include "phonecall_cmd.h"
#include "keybl_cmd.h"
#include "msensor_cmd.h"

#define LISTENER_UART "/dev/ttyS1"
#define QUIT_MSG "quit"
#define BUFLEN 512

#define CMD_MAX 50

struct _CmdListener {
	int uart_fd;
	int listener_start;
	int start_test;

	struct termios opt;
	struct pollfd* mfds;
	int ctrl_fd[2];
	char readbuf[BUFLEN];
	ssize_t read_len;
	Parcel* received_parcel;
	Parcel* trace;

	CmdInterface** cmds;
	int cmd_max_size;
	int cmd_count;

	Queue* cmd_queue;
};

static void cmd_listener_on_data_available(CmdListener* thiz);
static void cmd_listener_register_cmd(CmdListener* thiz, CmdInterface* cmd);
static CmdInterface* cmd_listener_find_cmd(CmdListener* thiz, uint8_t sub_cmd);
static void* cmd_listener_thread_run(void* ptr);
static void queue_parcel_destroy_func(void* ctx, void* data);

static void queue_parcel_destroy_func(void* ctx, void* data)
{
	Parcel* parcel = (Parcel*)data;

	parcel_destroy(parcel);
}

static void* cmd_listener_thread_run(void* ptr)
{
	CmdListener* thiz = (CmdListener*)ptr;
	int ret;

	while(thiz->listener_start == 1) {
		ret = poll(thiz->mfds, 2, -1);
		if (ret < 0) {
			LOGE("%s: poll ret < 0", __func__);
			continue;
		}

		if (thiz->mfds[0].revents == POLLIN) {
			LOGE("%s: pipe ctrl have POLLIN", __func__);
		}

		if (thiz->mfds[1].revents == POLLIN) {
			LOGE("%s: uart have POLLIN", __func__);
			cmd_listener_on_data_available(thiz);
		}
		
	}

	return NULL;
}

static void cmd_listener_register_cmd(CmdListener* thiz, CmdInterface* cmd)
{
	return_if_fail(cmd != NULL);

	if (thiz->cmd_max_size <= thiz->cmd_count) {
		LOGE("%s: realloc CmdInterface memory", __func__);
		thiz->cmds = (CmdInterface**) realloc(thiz->cmds, (thiz->cmd_max_size + CMD_MAX) * sizeof(CmdInterface*));
		thiz->cmd_max_size += CMD_MAX;
	}

	thiz->cmds[thiz->cmd_count] = cmd;
	thiz->cmd_count++;
}

static CmdInterface* cmd_listener_find_cmd(CmdListener* thiz, uint8_t sub_cmd)
{
	int i = 0;

	for (i = 0; i < thiz->cmd_count; i++) {
		if (cmd_interface_get_cmd(thiz->cmds[i]) == (int)sub_cmd) {
			LOGE("%s: get CmdInterface for cmd 0x%x", __func__, sub_cmd);
			return thiz->cmds[i];
		}
	}

	return NULL;
}

void cmd_listener_reply(CmdListener* thiz, Parcel* parcel)
{
	char* buf = NULL;
	size_t buf_len = 0;
	int i = 0;
	int ret;

	buf_len = parcel_get_buf(parcel, &buf);

	LOGE("%s: send reply", __func__);
	for (i = 0; i < (int)buf_len; i++) {
		LOGE("%s: buf[%d] = 0x%x", __func__, i, (uint8_t)buf[i]);
	} 

	ret = write(thiz->uart_fd, buf, buf_len);
	LOGE("%s: write ret is %d", __func__, ret);
}

// TODO: get cmd send to queue
static void cmd_listener_on_data_available(CmdListener* thiz)
{
	int ret;
	int i = 0;
	uint8_t main_cmd = 0;
	uint8_t sub_cmd = 0;
	memset(thiz->readbuf, 0, BUFLEN);
	thiz->read_len = 0;

	ret = read(thiz->uart_fd, thiz->readbuf, BUFLEN);
	if (ret < 0) {
		LOGE("%s: read error\n", __func__);
		return;
	}

	LOGE("%s: get a buf lenght %d", __func__, ret);
	thiz->read_len = ret;
	// add by a test
	for (i = 0; i < ret; i++) {
		LOGE("%s: readbuf[%d] = 0x%x", __func__, i, (uint8_t)thiz->readbuf[i]);
	}

	Parcel* receiver_p = parcel_create();
	if (parcel_set_buf(receiver_p, thiz->readbuf, thiz->read_len) != RET_OK) {
		return;
	}

	queue_push(thiz->cmd_queue, (void*)receiver_p);

	/*
	if (parcel_set_buf(thiz->received_parcel, thiz->readbuf, thiz->read_len) != RET_OK) {
		return;
	}

	parcel_get_main_cmd(thiz->received_parcel, &main_cmd);
	parcel_get_sub_cmd(thiz->received_parcel, &sub_cmd);
	LOGE("%s: get parcel's main cmd: 0x%x, sub cmd: 0x%x", __func__, 
			main_cmd, sub_cmd);

	if (main_cmd == DIAG_BOOT_MAIN_CMD && sub_cmd == DIAG_BOOT_SUB_CMD_2) {
		thiz->start_test = 1;
	}

	if (main_cmd == DIAG_AUTOTEST_F && thiz->start_test == 1) {
		//start auto test, do something if can not find a cmd
		// cmd ctx: include parcel content and its lenght or parcel 
		//cmd_listener_execute_cmd(thiz, sub_cmd);
		cmd_interface_execute(cmd_listener_find_cmd(thiz, sub_cmd), (void*)thiz->received_parcel);
		//queue_push(thiz->cmd_queue, (void*)sub_cmd);
	} */
}

void cmd_listener_start(CmdListener* thiz)
{
	return_if_fail(thiz != NULL);

	char boot_cmd[10] = {PACKAGE_HEAD,0x00,0x00,0x00,0x00,0x08,0x00,DIAG_BOOT_MAIN_CMD,DIAG_BOOT_SUB_CMD_1,PACKAGE_END};
	int ret;
	int i = 3;
	pthread_t t;
	thiz->listener_start = 1;
	Parcel* reciver;
	uint8_t main_cmd;
	uint8_t sub_cmd;

	// send boot cmd, may be by Parcel
	while (i > 0) {
		ret = write(thiz->uart_fd, boot_cmd, sizeof(boot_cmd));
		if (ret <= 0) {
			i--;
			usleep(300000);
			continue;
		}
		i = 0;
	} 

	LOGE("%s: **** after write boot cmd **** ", __func__);

	// start listener cmd in a thread
	pthread_create(&t, NULL, cmd_listener_thread_run, (void*)thiz);

	// get cmd from queue
	while(thiz->listener_start == 1) {
		if (queue_length(thiz->cmd_queue) == 0) {
			usleep(200000);
			continue;
		}

		queue_head(thiz->cmd_queue, (void**)&reciver);
		parcel_get_main_cmd(reciver, &main_cmd);
		parcel_get_sub_cmd(reciver, &sub_cmd);
		LOGE("%s: get parcel's main cmd: 0x%x, sub cmd: 0x%x", __func__, 
				main_cmd, sub_cmd);

		if (main_cmd == DIAG_BOOT_MAIN_CMD && sub_cmd == DIAG_BOOT_SUB_CMD_2) {
			thiz->start_test = 1;
		}

		if (main_cmd == DIAG_AUTOTEST_F && thiz->start_test == 1) {
			// cmd ctx: include parcel content and its lenght or parcel 
			cmd_interface_execute(cmd_listener_find_cmd(thiz, sub_cmd), (void*)reciver);
		}

		queue_pop(thiz->cmd_queue);
	}

	pthread_join(t, NULL);
}

void cmd_listener_quit(CmdListener* thiz)
{
	return_if_fail(thiz != NULL);

	thiz->listener_start = 0;
	write(thiz->ctrl_fd[1], QUIT_MSG, strlen(QUIT_MSG));
}

void cmd_listener_send_trace(CmdListener* thiz, char* trace)
{
	return_if_fail(thiz != NULL && trace != NULL);

	int length = strlen(trace);
	char* buf = malloc(length + 5);
	memset(buf, 0, length + 5);
	memcpy(buf + 4, trace, length);

	parcel_set_content(thiz->trace, buf, (size_t)(length + 5));
	cmd_listener_reply(thiz, thiz->trace);

	free(buf);
}

void cmd_listener_destroy(CmdListener* thiz)
{
	return_if_fail(thiz != NULL);

	queue_destroy(thiz->cmd_queue);
	close(thiz->ctrl_fd[0]);
	close(thiz->ctrl_fd[1]);

	parcel_destroy(thiz->received_parcel);
	parcel_destroy(thiz->trace);
	SAFE_FREE(thiz->mfds);
	SAFE_FREE(thiz);
}

CmdListener* cmd_listener_create(void)
{
	CmdListener* thiz = (CmdListener*) malloc(sizeof(CmdListener));

	if (thiz != NULL) {
		thiz->cmd_queue = queue_create(queue_parcel_destroy_func, NULL);

		thiz->listener_start = 0;
		thiz->start_test = 0;
		thiz->uart_fd = open(LISTENER_UART, O_RDWR);
		if (thiz->uart_fd < 0) {
			LOGE("%s: fail to open %s", __func__, LISTENER_UART);
			return NULL;
		}

		// uart init
		tcgetattr(thiz->uart_fd, &thiz->opt);
		tcflush(thiz->uart_fd, TCIFLUSH);
		cfsetispeed(&thiz->opt, B115200);
		cfsetospeed(&thiz->opt, B115200);

		thiz->opt.c_cflag &= ~PARENB;
		thiz->opt.c_cflag &= ~CSTOPB;
		thiz->opt.c_cflag &= ~CSIZE;
		thiz->opt.c_cflag |= CS8;
		thiz->opt.c_lflag &= ~(ICANON|ISIG|ECHO|IEXTEN);
		thiz->opt.c_oflag &= ~OPOST;
		thiz->opt.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
		thiz->opt.c_cc[VMIN] = 0;
		thiz->opt.c_cc[VTIME] = 0;

		if (tcsetattr(thiz->uart_fd, TCSANOW, &thiz->opt) != 0) {
			LOGE("%s: uart init fail", __func__);
			return NULL;
		}

		thiz->mfds = (struct pollfd*)calloc(2, sizeof(struct pollfd));
		pipe(thiz->ctrl_fd);
		thiz->mfds[0].fd = thiz->ctrl_fd[0];
		thiz->mfds[0].events = POLLIN;
		thiz->mfds[1].fd = thiz->uart_fd;
		thiz->mfds[1].events =POLLIN;

		memset(thiz->readbuf, 0, sizeof(thiz->readbuf));
		thiz->read_len = 0;

		thiz->received_parcel = parcel_create();
		thiz->trace = parcel_create();
		parcel_set_main_cmd(thiz->trace, DIAG_AUTOTEST_F);
		parcel_set_sub_cmd(thiz->trace, AUTOTEST_TRACE);

		//register all cmd
		thiz->cmds = (CmdInterface**) calloc(CMD_MAX, sizeof(CmdInterface*));
		thiz->cmd_count = 0;
		thiz->cmd_max_size = CMD_MAX;

		cmd_listener_register_cmd(thiz, version_info_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, getoption_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, backlight_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, echoloop_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, echoloop2_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, keypress_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, tp_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, vibrator_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, receive_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, lcd_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, ring_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, headset_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, cft_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, flashlight_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, led_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, pl_sensor_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, charge_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, fm_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, memorycard_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, rtc_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, camera_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, camera1_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, gsensor_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, sim_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, bluetooth_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, wifi_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, over_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, phonecall_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, keybl_cmd_create(thiz));
		cmd_listener_register_cmd(thiz, msensor_cmd_create(thiz));
	}

	return thiz;
}

