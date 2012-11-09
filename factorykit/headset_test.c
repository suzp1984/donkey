#include "headset_test.h"
#include "ui.h"
#include "minui.h"
#include "common.h"

#include <stdlib.h>
#include <sys/select.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#define SPRD_HEADSET_SWITCH_DEV         "/sys/class/switch/h2w/state"
#define SPRD_HEASETKEY_DEV              "headset-keyboard"
#define SPRD_HEADSET_KEY                KEY_MEDIA
#define SPRD_HEADSET_KEYLONGPRESS       KEY_END

#define SPRD_HEADSETOUT                 "HEADSET OUT"
#define SPRD_HEADSETIN                  "HEADSET IN"
#define SPRD_HEADSET_KEYPRESS           "HEADSET_KEY PRESS"
#define SPRD_HEADSET_KEYNOTPRESS        "HEADSET_KEY NOT PRESS"
#define SPRD_HEADSET_LOOPBACK			"HEASSET AUDIO LOOPBACK"

#define DEFAULT_RATE 44100
#define DEFAULT_FRAMES 32

#define LOG_TAG "factorykit"
#include <utils/Log.h>

typedef struct {
	int headset_status;
	int headset_key;
	int thread_run;

	int frames;
	snd_pcm_t* playback_handle;
	snd_pcm_t* capture_handle;
} PrivInfo;

static void* headset_test_check_thread(void* ctx);
static void* headset_test_loopback(void* ctx);
static void headset_test_show(TestCase* thiz);
static int headset_test_set_capture_handle(TestCase* thiz);
static int headset_test_set_playback_handle(TestCase* thiz);
static int headset_test_audio_loopback(TestCase* thiz);

static int headset_test_audio_loopback(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	int i = 0;
	int rc = 0;
	int size = 0;
	char* buffer = NULL;
 
	system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Earpiece Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Headset Playback Switch\" 1");
	system("alsa_amixer cset -c sprdphone name=\"PCM2 Playback Switch\" 1");
	system("alsa_amixer cset -c sprdphone name=\"PCM Playback Switch\" 1");
	usleep(10000);
	system("alsa_amixer sset 'Micphone' 2");
	system("alsa_amixer sset \"PCM2\" 60%");
	system("alsa_amixer cset -c sprdphone name='Capture Capture Volume' 80%");
	usleep(10000);

	/* set capture handle */
	for (i = 0;  i < 5; i++) {
		if ((rc = headset_test_set_capture_handle(thiz)) == 0) {
			break;
		}

		LOGE("%s: set capture handle fail", __func__);
		snd_pcm_drain(priv->capture_handle);
		snd_pcm_close(priv->capture_handle);
		usleep(100000);
	}

	LOGE("%s: success set capture_handle (%d)", __func__, i);
	if (i == 5) {
		goto out;
	}

	/* set playback handle */
	for (i = 0; i < 5; i++) {
		if ((rc = headset_test_set_playback_handle(thiz)) == 0) {
			break;
		}

		LOGE("%s: set playback handle fail", __func__);
		snd_pcm_drain(priv->playback_handle);
		snd_pcm_close(priv->playback_handle);
		usleep(100000);
	}

	LOGE("%s: success set playback_handle(%d)", __func__, i);
	if (i == 5) {
		goto out;
	}

	size = priv->frames * 4; // 2bytes/sample, 2 channels
	buffer = (char*)malloc(size);
	LOGE("%s: frames = %d; size = %d", __func__, priv->frames, size);

	while(priv->headset_key > 0) {
		memset(buffer, 0, size);
		rc = snd_pcm_readi(priv->capture_handle, buffer, priv->frames);
		if (rc == -EPIPE) {
			LOGE("%s: capture overrun occurred", __func__);
			snd_pcm_prepare(priv->capture_handle);
		}

		while((rc = snd_pcm_writei(priv->playback_handle, buffer, priv->frames)) < 0) {
			usleep(2000);
			if (rc == -EPIPE) {
				LOGE("%s: playback overrun occurred", __func__);
				snd_pcm_prepare(priv->playback_handle);
			} else if (rc < 0) {
				LOGE("%s: error from writei: %s", __func__,
						snd_strerror(rc));
			}
		}
	}

	free(buffer);

out:
	rc = snd_pcm_drain(priv->playback_handle);
	rc = snd_pcm_close(priv->playback_handle);
	rc = snd_pcm_drain(priv->capture_handle);
	rc = snd_pcm_close(priv->capture_handle);

	system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Earpiece Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Headset Playback Switch\" 0");
	system("alsa_amixer sset 'Micphone' 1");
	sleep(1);
	system("alsa_amixer -c sprdphone cset name='Power Codec' 4");


	return 0;
}

static int headset_test_set_capture_handle(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int rc;
	unsigned int val;
	int dir;

	snd_pcm_hw_params_t* params;
	snd_pcm_uframes_t frames;

	/* open PCM device for capture */
	rc = snd_pcm_open(&priv->capture_handle, "plughw:sprdphone",
			SND_PCM_STREAM_CAPTURE, 0);

	if (rc < 0) {
		LOGE("%s: unable to open pcm device: %s", __func__,
				snd_strerror(rc));
		return -2;
	}

	/* Allocate a hardware parameters object */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it with default value */
	snd_pcm_hw_params_any(priv->capture_handle, params);

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(priv->capture_handle, params,
			SND_PCM_ACCESS_RW_INTERLEAVED);

	/*signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(priv->capture_handle, params,
			SND_PCM_FORMAT_S16_LE);

	/*Two channels (stereo) */
	snd_pcm_hw_params_set_channels(priv->capture_handle, params, 2);

	/* 44100bits/seconds sampling rate (CD quality) */
	val = DEFAULT_RATE;
	snd_pcm_hw_params_set_rate_near(priv->capture_handle, params,
			&val, &dir);

	/* Set period size to 32 frames */
	frames = DEFAULT_FRAMES;
	snd_pcm_hw_params_set_period_size_near(priv->capture_handle,
			params, &frames, &dir);

	rc = snd_pcm_hw_params(priv->capture_handle, params);
	if (rc < 0) {
		LOGE("%s: unable to set hw parameters: %s", __func__, snd_strerror(rc));
		return -1;
	}

	return 0;
}

static int headset_test_set_playback_handle(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int rc;
	unsigned int val;
	int dir;
	int i = 0;

	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;

	/* open PCM driver for playback */
	for (i = 0; i < 5; i++) {
		rc = snd_pcm_open(&priv->playback_handle, "plughw:sprdphone",
				SND_PCM_STREAM_PLAYBACK, 0);

		if (rc == 0) {
			break;
		}

		if (rc < 0) {
			LOGE("%s: unable to open pcm device: %s", __func__,
					snd_strerror(rc));

		}

		usleep(100000);
	}

	if (i == 5) {
		return -1;
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(priv->playback_handle, params);

	/* Interleaved mode */
	rc = snd_pcm_hw_params_set_access(priv->playback_handle, params,
			SND_PCM_ACCESS_RW_INTERLEAVED);
	if (rc < 0) {
		return -1;
	}

	/*signed 16-bit little-endian format */
	rc = snd_pcm_hw_params_set_format(priv->playback_handle, params,
			SND_PCM_FORMAT_S16_LE);
	if (rc < 0) {
		return -1;
	}

	/*Two channels (stereo) */
	rc = snd_pcm_hw_params_set_channels(priv->playback_handle, params, 2);
	if (rc < 0) {
		return -1;
	}

	/* 44100bits/seconds sampling rate (CD quality) */
	val = DEFAULT_RATE;
	rc = snd_pcm_hw_params_set_rate_near(priv->playback_handle, params,
			&val, &dir);
	if (rc < 0) {
		return -1;
	}

	/* Set period size to 32 frames */
	frames = DEFAULT_FRAMES;
	rc = snd_pcm_hw_params_set_period_size_near(priv->playback_handle,
			params, &frames, &dir);
	if (rc < 0) {
		return -1;
	}

	/* write parameters to the driver */
	rc = snd_pcm_hw_params(priv->playback_handle, params);
	if (rc < 0) {
		LOGE("%s: unable to set hw parameters: %s", __func__, snd_strerror(rc));
		return -1;
	}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	priv->frames = (int)frames;

	return 0;
}

static void* headset_test_loopback(void* ctx)
{
	TestCase* thiz = (TestCase*)ctx;
	DECLES_PRIV(priv, thiz);

	while(priv->thread_run == 1) {
		if (priv->headset_key > 0) {
			headset_test_audio_loopback(thiz);
		}
		usleep(100000);
	}

	return NULL;
}

static void headset_test_show(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);

	gr_color(0, 255, 0, 255);
	ui_draw_title(UI_TITLE_START_Y, HEADSET_TEST_CASE);

	if (priv->headset_status == 0) {
		priv->headset_key = -1;
		gr_color(0, 0, 0, 255);
		ui_draw_title(gr_fb_height()/3, SPRD_HEADSETIN);
		gr_color(255, 0, 0, 255);
		ui_draw_title(gr_fb_height()/3, SPRD_HEADSETOUT);
	} else if (priv->headset_status > 0) {
		gr_color(0, 0, 0, 255);
		ui_draw_title(gr_fb_height()/3, SPRD_HEADSETOUT);
		gr_color(0, 255, 0, 255);
		ui_draw_title(gr_fb_height()/3, SPRD_HEADSETIN);
	}

	if (priv->headset_key <= 0) {
		gr_color(0, 0, 0, 255);
		ui_draw_title(gr_fb_height()/3 + CHAR_HEIGHT, SPRD_HEADSET_KEYPRESS);
		ui_draw_title(gr_fb_height()/3 + 2*CHAR_HEIGHT, SPRD_HEADSET_LOOPBACK);
		gr_color(255, 0, 0, 255);
		ui_draw_title(gr_fb_height()/3 + CHAR_HEIGHT, SPRD_HEADSET_KEYNOTPRESS);
	} else {
		gr_color(0, 0, 0, 255);
		ui_draw_title(gr_fb_height()/3 + CHAR_HEIGHT, SPRD_HEADSET_KEYNOTPRESS);
		gr_color(0, 255, 0, 255);
		ui_draw_title(gr_fb_height()/3 + CHAR_HEIGHT, SPRD_HEADSET_KEYPRESS);
		ui_draw_title(gr_fb_height()/3 + 2*CHAR_HEIGHT, SPRD_HEADSET_LOOPBACK);
	}

	gr_flip();
}

static void* headset_test_check_thread(void* ctx)
{
	TestCase* thiz = (TestCase*)ctx;
	DECLES_PRIV(priv, thiz);

	int counter;
	int fd, ret, n, nread;
	int input_fd = -1;
	fd_set rfds;
	char buffer[8];
	struct timeval timeout;
	struct input_event event;

	memset(&timeout, 0, sizeof(timeout));
	timeout.tv_sec = 0;
	timeout.tv_usec = 500*1000;

	fd = open(SPRD_HEADSET_SWITCH_DEV, O_RDONLY);
	if (fd < 0) {
		LOGE("%s: open %s fail", __func__, SPRD_HEADSET_SWITCH_DEV);
		goto out;
	}

	input_fd = open_input(SPRD_HEASETKEY_DEV, O_RDONLY);
	if (input_fd < 0) {
		LOGE("%s: can not open input device %s", __func__, SPRD_HEASETKEY_DEV);
		goto out;
	}

	while(priv->thread_run == 1) {
		memset(buffer, 0, sizeof(buffer));
		lseek(fd, 0, SEEK_SET);
		ret = read(fd, buffer, sizeof(buffer));
		if (ret < 0) {
			usleep(1000000);
			continue;
		}

		priv->headset_status = atoi(buffer);

		if (priv->headset_status > 0 && priv->headset_key != 1) {
			FD_ZERO(&rfds);
			FD_SET(input_fd, &rfds);
			n = select(input_fd+1, &rfds, NULL, NULL, &timeout);

			if (FD_ISSET(input_fd, &rfds)) {
				nread = read(input_fd, &event, sizeof(event));
				if (nread == sizeof(event)) {
					if (event.type == EV_KEY) {
						if (event.value > 0) {
							if ((event.code == SPRD_HEADSET_KEY) || (event.code == SPRD_HEADSET_KEYLONGPRESS)) {
								priv->headset_key = 1;
							}
						}
					}
				}
			}

		}

		headset_test_show(thiz);
		usleep(200000);
	}

out:
	if (fd >= 0) {
		close(fd);
	}

	if (input_fd >= 0) {
		close(input_fd);
	}

	return NULL;
}

static int headset_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	pthread_t t_check;
	pthread_t t_loopback;

	ui_screen_clean();

	fk_stop_service("media");
	usleep(100000);
	headset_test_show(thiz);
	priv->headset_key = 0;
	priv->headset_status = 0;

	priv->thread_run = 1;
	pthread_create(&t_check, NULL, headset_test_check_thread, (void*)thiz);
	pthread_create(&t_loopback, NULL, headset_test_loopback, (void*)thiz);

	thiz->passed = ui_draw_handle_softkey(thiz->name);

	LOGE("%s: after ui_draw_handle_softkey", __func__);
	priv->thread_run = 0;
	priv->headset_status = 0;
	priv->headset_key = -1;
	pthread_join(t_check, NULL);
	pthread_join(t_loopback, NULL);

	return 0;
}

static void headset_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* headset_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = headset_test_run;
		thiz->destroy = headset_test_destroy;

		thiz->name = HEADSET_TEST_CASE;
		thiz->passed = passed;
		priv->headset_status = 0;
		priv->headset_key = 0;
		priv->thread_run = 0;
	}

	return thiz;
}
