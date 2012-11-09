#include "phone_loopback_test_case.h"
#include "ui.h"
#include "minui.h"

#include "common.h"
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

#define LOG_TAG "phoneloopback"
#include <utils/Log.h>

#define DEFAULT_RATE 44100
#define DEFAULT_FRAMES 32

typedef struct {
	int thread_run;
	snd_pcm_t* playback_handle;
	snd_pcm_t* capture_handle;
	int frames;
} PrivInfo;

static int phone_audio_playback(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	int rc;
	unsigned int val;
	int dir;

	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;

	/* open PCM driver for playback */
	rc = snd_pcm_open(&priv->playback_handle, "plughw:sprdphone",
			SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		LOGE("%s: unable to open pcm device: %s", __func__, 
				snd_strerror(rc));
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
		LOGE("%s: snd_pcm_hw_params_set_access fail(%s)", __func__, snd_strerror(rc));
		return -1;
	}

	/*signed 16-bit little-endian format */
	rc = snd_pcm_hw_params_set_format(priv->playback_handle, params,
			SND_PCM_FORMAT_S16_LE);
	if (rc < 0) {
		LOGE("%s: snd_pcm_hw_params_set_format fail (%s)", __func__, snd_strerror(rc));
		return -1;
	}

	/*Two channels (stereo) */
	rc = snd_pcm_hw_params_set_channels(priv->playback_handle, params, 2);
	if (rc < 0) {
		LOGE("%s: snd_pcm_hw_params set channels error(%s)", __func__, snd_strerror(rc));
		return -1;
	}
	
	/* 44100bits/seconds sampling rate (CD quality) */
	val = DEFAULT_RATE;
	rc = snd_pcm_hw_params_set_rate_near(priv->playback_handle, params, 
			&val, &dir);

	/* Set period size to 32 frames */
	frames = DEFAULT_FRAMES;
	rc = snd_pcm_hw_params_set_period_size_near(priv->playback_handle, 
			params, &frames, &dir);

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

static int phone_audio_capture(TestCase* thiz)
{
	int rc;
	unsigned int val;
	int dir;

	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;

	DECLES_PRIV(priv, thiz);

	/* open PCM driver for capture. */
	rc = snd_pcm_open(&priv->capture_handle, "plughw:sprdphone",
			SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		LOGE("%s: unable to open pcm device: %s", __func__, 
				snd_strerror(rc));
		return -1;
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
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

	rc = snd_pcm_hw_params(priv->capture_handle, params);
	if (rc < 0) {
		LOGE("%s: unable to set hw parameters: %s", __func__, snd_strerror(rc));
		return -1;
	}

	return 0;
}

static void* phone_loopback_thread_routine(void* ctx)
{
	TestCase* thiz = (TestCase*)ctx;
	char* buffer;
	int size;
	int rc;
	int i = 0;

	DECLES_PRIV(priv, thiz);

	system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0"); 
	system("alsa_amixer cset -c sprdphone name=\"Earpiece Playback Switch\" 1"); 
	system("alsa_amixer cset -c sprdphone name=\"Headset Playback Switch\" 0");
	//system("alsa_amixer cset -c sprdphone name=\"PCM2 Playback Switch\" 1");
	//system("alsa_amixer cset -c sprdphone name=\"PCM Playback Switch\" 1");
	usleep(10000);
	system("alsa_amixer sset 'Micphone' 1"); 
	system("alsa_amixer sset \"PCM2\" 46%");
	system("alsa_amixer cset -c sprdphone name='Capture Capture Volume' 80%"); 
	usleep(10000);

	//set_audio_mode(0x1);

	LOGE("%s: set capture params", __func__);
	//set capture params
	for (i = 0; i < 5; i++) {
		if (phone_audio_capture(thiz) == 0) {
			break;
		}

		LOGE("%s: set capture handle fail", __func__);
		snd_pcm_drain(priv->capture_handle);
		snd_pcm_close(priv->capture_handle);
		usleep(100000);
	}

	if (i == 5) {
		goto out_capture;
	}

	LOGE("%s: set playback params", __func__);
	//set playback params
	for (i = 0; i < 5; i++) {
		if (phone_audio_playback(thiz) == 0) {
			break;
		}

		LOGE("%s: set playback handle fail", __func__);
		snd_pcm_drain(priv->playback_handle);
		snd_pcm_close(priv->playback_handle);
		usleep(100000);
	}

	if (i == 5) {
		goto out_playback;
	}

	size = priv->frames * 4; // 2 bytes/sample, 2 channels 
	buffer = (char*) malloc(size);

	LOGE("%s: frames = %d; size = %d", __func__, priv->frames, size);

	if (priv->frames <= 0 || size <= 0) 
		return NULL;

	while(priv->thread_run == 1) {
		memset(buffer, 0, size);
		
		rc = snd_pcm_readi(priv->capture_handle, buffer, priv->frames);
		if (rc == -EPIPE) {
			LOGE("%s: capture overrun occurred", __func__);
			snd_pcm_prepare(priv->capture_handle);
		}

		LOGE("%s: ** pcm_writei **", __func__);

		while ((rc = snd_pcm_writei(priv->playback_handle, buffer, priv->frames)) < 0) {
			usleep(3000);
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

	rc = snd_pcm_drain(priv->playback_handle);
	rc = snd_pcm_close(priv->playback_handle);
out_playback:
	rc = snd_pcm_drain(priv->capture_handle);
	rc = snd_pcm_close(priv->capture_handle);

out_capture:
	system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Earpiece Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Headset Playback Switch\" 0");
	sleep(1);
	system("alsa_amixer -c sprdphone cset name='Power Codec' 4");

	return NULL;
}

static int phone_loopback_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	pthread_t t;
	int ret;

	ui_screen_clean();

	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, PHONE_LOOPBACK_TEST_CASE);
	gr_flip();

	fk_stop_service("media");

	priv->thread_run = 1;
	pthread_create(&t, NULL, phone_loopback_thread_routine, (void*)thiz);

	ret = ui_draw_handle_softkey(thiz->name);
	thiz->passed = ret;

	priv->thread_run = 0;
	pthread_join(t, NULL);

	return 0;
}

static void phone_loopback_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* phone_loopback_test_create(int passed)
{
	TestCase* thiz = (TestCase*) malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);

		thiz->run = phone_loopback_test_run;
		thiz->destroy = phone_loopback_test_destroy;

		thiz->passed = passed;
		thiz->name = PHONE_LOOPBACK_TEST_CASE;

		priv->thread_run = 0;
		priv->frames = 0;
	}

	return thiz;
}
