#include "fm_test.h"
#include "ui.h"
#include "minui.h"
#include "common.h"

#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define SPRD_FM_DEV             "/dev/rda5802_fm"
#define SPRD_HEADSET_SWITCH_DEV "/sys/class/switch/h2w/state"
#define DEFAULT_TUNE 875
#define SPRD_HEADSETDOUT        "HEADSET OUT"
#define SPRD_HEADSETIN          "HEADSET IN"
#define SPRD_FM_ERROR           "Fail: FM"
#define SPRD_FM_OK              "Pass: FM"

#define KT0812G_FM_IOCTL_BASE     'R'
#define KT0812G_FM_IOCTL_ENABLE      _IOW(KT0812G_FM_IOCTL_BASE, 0, int)
#define KT0812G_FM_IOCTL_GET_ENABLE  _IOW(KT0812G_FM_IOCTL_BASE, 1, int)
#define KT0812g_FM_IOCTL_SET_TUNE    _IOW(KT0812G_FM_IOCTL_BASE, 2, int)
#define KT0812g_FM_IOCTL_GET_FREQ    _IOW(KT0812G_FM_IOCTL_BASE, 3, int)
#define KT0812G_FM_IOCTL_SEARCH      _IOW(KT0812G_FM_IOCTL_BASE, 4, int[4])
#define KT0812G_FM_IOCTL_STOP_SEARCH _IOW(KT0812G_FM_IOCTL_BASE, 5, int)
#define KT0812G_FM_IOCTL_MUTE        _IOW(KT0812G_FM_IOCTL_BASE, 6, int)
#define KT0812G_FM_IOCTL_SET_VOLUME  _IOW(KT0812G_FM_IOCTL_BASE, 7, int)
#define KT0812G_FM_IOCTL_GET_VOLUME  _IOW(KT0812G_FM_IOCTL_BASE, 8, int)

enum {
	FMTEST_IDLE,
	FMTEST_PLAY,
	FMTEST_STOP,
	FMTEST_PLAY_ERR,
};

enum { 
	FMTEST_ERR_FAIL,
	FMTEST_ERR_SUCCESS,
};

typedef struct {
	int headset_in;
	int fm_status;
	int thread_run;
} PrivInfo;

static int fm_test_play(int start)
{
	static int fd = -1;
	int on_off;
	int buffer[4];
	int ret = FMTEST_ERR_FAIL;
	int default_tune = DEFAULT_TUNE;

	if (start == 1) {
		LOGE("%s: FM play start", __func__);
		fd = open(SPRD_FM_DEV, O_RDWR);
		if (fd < 0) {
			LOGE("%s: open %s fail", __func__, SPRD_FM_DEV);
			return FMTEST_ERR_FAIL;
		}

		system("alsa_amixer cset -c sprdphone name=\"BypassFM Playback Switch\" 1");
		system("alsa_amixer cset -c sprdphone name=\"Headset Playback Switch\" 1");
		system("alsa_amixer sset 'LineinFM' on");
		system("alsa_amixer -c sprdphone cset name='Power Codec' 1");

		if (ioctl(fd, KT0812G_FM_IOCTL_ENABLE, &start) < 0) {
			LOGE("%s: ioctl open fm fail [%s]", __func__, strerror(errno));
			return FMTEST_ERR_FAIL;
		}

		if (ioctl(fd, KT0812g_FM_IOCTL_SET_TUNE, &default_tune) < 0) {
			LOGE("%s: ioctl set fm tune [%d] fail [%s]", __func__, default_tune, strerror(errno));
			return FMTEST_ERR_FAIL;
		}
	} else {
		LOGE("%s: FM play stop", __func__);
		system("alsa_amixer -c sprdphone cset name='Power Codec' 4");
		system("alsa_amixer cset -c sprdphone name=\"BypassFM Playback Switch\" 0");
		system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
		system("alsa_amixer sset 'LineinFM' off");

		if (ioctl(fd, KT0812G_FM_IOCTL_ENABLE, &start) < 0) {
			LOGE("%s: disable FM error", __func__);
			return FMTEST_ERR_FAIL;
		}
	}

	return FMTEST_ERR_SUCCESS;
}

static void* fm_test_headset_check_thread(void* ctx)
{
	TestCase* thiz = (TestCase*)ctx;
	DECLES_PRIV(priv, thiz);
	int fd;
	int ret;
	char buffer[8];

	fd = open(SPRD_HEADSET_SWITCH_DEV, O_RDONLY);
	if (fd < 0) {
		LOGE("%s: open %s fail", __func__, SPRD_HEADSET_SWITCH_DEV);
		return NULL;
	}

	while(priv->thread_run == 1) {
		memset(buffer, 0, sizeof(buffer));
		lseek(fd, 0, SEEK_SET);
		ret = read(fd, buffer, sizeof(buffer));
		if (ret < 0) {
			LOGE("%s: read fd fail error[%d]=%s", __func__, errno, strerror(errno));
		}

		priv->headset_in = atoi(buffer);
		if (priv->headset_in) {
			if (priv->fm_status == FMTEST_PLAY) continue;

			/* FM play start here */
			if (fm_test_play(1) == FMTEST_ERR_SUCCESS) {
				priv->fm_status = FMTEST_PLAY;
			} else {
				priv->fm_status = FMTEST_PLAY_ERR;
			}
		} else {
			if (priv->fm_status == FMTEST_PLAY && fm_test_play(0) == FMTEST_ERR_SUCCESS) {
				priv->fm_status = FMTEST_STOP;
			}
		}

		usleep(100000);
	}
	
	return NULL;
}

static void* fm_test_show_thread(void* ctx)
{
	TestCase* thiz = (TestCase*)ctx;
	DECLES_PRIV(priv, thiz);
	int start_y = gr_fb_height() / 4;

	gr_color(0, 255, 0, 255);
	ui_draw_title(UI_TITLE_START_Y, FM_TEST_CASE);

	while(priv->thread_run == 1) {

		if(priv->headset_in == 0) {
			gr_color(0, 0, 0, 255);
			ui_draw_title(start_y, SPRD_HEADSETIN);
			gr_color(255, 0, 0, 255);
			ui_draw_title(start_y, SPRD_HEADSETDOUT);
		} else {
			gr_color(0, 0, 0, 255);
			ui_draw_title(start_y, SPRD_HEADSETDOUT);
			gr_color(0, 255, 0, 255);
			ui_draw_title(start_y, SPRD_HEADSETIN);
		}

		if (priv->fm_status == FMTEST_PLAY_ERR) {
			gr_color(0, 0, 0, 255);
			ui_draw_title(start_y+CHAR_HEIGHT, SPRD_FM_OK);
			ui_draw_title(start_y+2*CHAR_HEIGHT, "set tune 87.5MHz");

			gr_color(255, 0, 0, 255);
			ui_draw_title(start_y+CHAR_HEIGHT, SPRD_FM_ERROR);
		} else if (priv->fm_status == FMTEST_PLAY) {
			gr_color(0, 0, 0, 255);
			ui_draw_title(start_y+CHAR_HEIGHT, SPRD_FM_ERROR);

			gr_color(0, 255, 0, 255);
			ui_draw_title(start_y+CHAR_HEIGHT, SPRD_FM_OK);
			ui_draw_title(start_y+2*CHAR_HEIGHT, "set tune 87.5MHz");
		} else if (priv->fm_status == FMTEST_STOP) {
			gr_color(0, 0, 0, 255);
			ui_draw_title(start_y+CHAR_HEIGHT, SPRD_FM_OK);
			ui_draw_title(start_y+2*CHAR_HEIGHT, "set tune 87.5MHz");
		}

		gr_flip();
		usleep(200000);
	}

	return NULL;
}

static int fm_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	fk_stop_service("media");
	priv->headset_in = 0;
	priv->fm_status = 0;

	pthread_t check_id;
	pthread_t fmtest_id;
	priv->thread_run = 1;
	ui_screen_clean();
	pthread_create(&check_id, NULL, fm_test_headset_check_thread, (void*)thiz);
	pthread_create(&fmtest_id, NULL, fm_test_show_thread, (void*)thiz);

	thiz->passed = ui_draw_handle_softkey(thiz->name);

	priv->thread_run = 0;
	pthread_join(check_id, NULL);
	pthread_join(fmtest_id, NULL);

	if (priv->fm_status == FMTEST_PLAY) {
		fm_test_play(0);
	}

	return 0;
}

static void fm_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* fm_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = fm_test_run;
		thiz->destroy = fm_test_destroy;

		thiz->name = FM_TEST_CASE;
		thiz->passed = passed;
		priv->headset_in = 0;
		priv->fm_status = 0;
		priv->thread_run = 0;
	}

	return thiz;
}
