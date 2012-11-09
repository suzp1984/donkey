#include "echoloop_cmd.h"
#include "cmd_common.h"
#include "parcel.h"
#include "utils.h"

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <string.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define DEFAULT_RATE 44100
#define DEFAULT_FRAME 32

typedef struct {
	CmdListener* listener;

	int thread_run;
	int frames;
	snd_pcm_t* playback_handle;
	snd_pcm_t* capture_handle;
	unsigned int default_rate;
} PrivInfo;

static void echoloop_cmd_set_capture_handle(CmdInterface* thiz)
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
		return;
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
	frames = DEFAULT_FRAME;
	snd_pcm_hw_params_set_period_size_near(priv->capture_handle,
			params, &frames, &dir);


	rc = snd_pcm_hw_params(priv->capture_handle, params);
	if (rc < 0) {
		LOGE("%s: unable to set hw parameters: %s", __func__, snd_strerror(rc));
	}
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	priv->frames = (int)frames;
	LOGE("%s: get capture_handle period size is %d", __func__, (int)frames);
	snd_pcm_hw_params_get_rate(params, &priv->default_rate, &dir);
	LOGE("%s: get capture_handle rate is %d", __func__, priv->default_rate);
}

static int echoloop_cmd_set_playback_handle(CmdInterface* thiz)
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
	rc = snd_pcm_hw_params_any(priv->playback_handle, params);
	if (rc < 0) {
		return -1;
	}

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
	val = priv->default_rate;
	rc = snd_pcm_hw_params_set_rate_near(priv->playback_handle, params,
			&val, &dir);
	if (rc < 0) {
		return -1;
	}

	/* Set period size to 32 frames */
	frames = priv->frames;
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
	snd_pcm_hw_params_get_rate(params, &priv->default_rate, &dir);
	LOGE("%s: get playback_handle rate is %d", __func__, priv->default_rate);

	return 0;
}

static void* echoloop_cmd_thread_run(void* ctx)
{
	CmdInterface* thiz = (CmdInterface*)ctx;
	DECLES_PRIV(priv, thiz);
	char* buffer;
	int size;
	int rc;
	int i = 0;

	system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Earpiece Playback Switch\" 1");
	system("alsa_amixer cset -c sprdphone name=\"Headset Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"PCM2 Playback Switch\" 1");
	system("alsa_amixer cset -c sprdphone name=\"PCM Playback Switch\" 1");
	system("alsa_amixer sset 'Micphone' 1");
	system("alsa_amixer sset \"PCM2\" 90%");
	system("alsa_amixer cset -c sprdphone name='Capture Capture Volume' 100%");
	usleep(100000);

	LOGE("%s: set alsa switch done", __func__);

	/* set capture handle */
	echoloop_cmd_set_capture_handle(thiz);

	/* set playback handle */
	for (i = 0; i < 5; i++) {
		if (echoloop_cmd_set_playback_handle(thiz) == 0) {
			break;
		}
		snd_pcm_drain(priv->playback_handle);
		snd_pcm_close(priv->playback_handle);
		usleep(100000);
	}

	if (i == 5) {
		goto out;
	}

	size = priv->frames * 4; // 2 bytes/sample, 2 channels
	buffer = (char*)malloc(size);
	LOGE("%s: frames = %d; size = %d", __func__, priv->frames, size);

	while (priv->thread_run == 1) {
		memset(buffer, 0, size);

		rc = snd_pcm_readi(priv->capture_handle, buffer, priv->frames);
		if (rc == -EPIPE) {
			LOGE("%s: capture overrun occurred", __func__);
			snd_pcm_prepare(priv->capture_handle);
		}

		while((rc = snd_pcm_writei(priv->playback_handle, buffer, priv->frames)) < 0) {
			usleep(1000);
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
out:
	rc = snd_pcm_drain(priv->capture_handle);
	rc = snd_pcm_close(priv->capture_handle);

	system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Earpiece Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Headset Playback Switch\" 0");
	sleep(1);
	system("alsa_amixer -c sprdphone cset name='Power Codec' 4");

	return NULL;
}

static int echoloop_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	char content[4];
	pthread_t t;
	
	Parcel* reply = parcel_create();

	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_ECHOLOOP);

	stop_service("media");
	usleep(100000);
	priv->thread_run = 1;
	pthread_create(&t, NULL, echoloop_cmd_thread_run, (void*)thiz);
	
	sleep(1);
	memset(content, 0, sizeof(content));
	// reply AUTOTEST_RET_STEP1
	*(uint8_t*)(content) = AUTOTEST_RET_STEP1;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);
	usleep(600000);
	// done
	*(uint8_t*)(content) = AUTOTEST_RET_DONE;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	priv->thread_run = 0;
	pthread_join(t, NULL);

	parcel_destroy(reply);
	return 0;
}

static void echoloop_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* echoloop_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = echoloop_cmd_execute;
		thiz->destroy = echoloop_cmd_destroy;

		thiz->cmd = AUTOTEST_ECHOLOOP;
		priv->listener = listener;
		priv->thread_run = 0;
		priv->frames = 0;
		priv->default_rate = 0;
	}

	return thiz;
}
