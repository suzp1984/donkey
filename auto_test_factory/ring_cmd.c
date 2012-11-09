#include "ring_cmd.h"
#include "cmd_common.h"
#include "parcel.h"
#include "utils.h"

#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

#define LOG_TAG "auto_test"
#include <utils/Log.h>

#define DEFAULT_RATE 48000
#define DEFAULT_SINE_FREQ 300
#define DEFAULT_NPERIODS 4
#define DEFAULT_CHANNELS 2

typedef struct {
	CmdListener* listener;

	int thread_run;
	snd_pcm_t* playback_handle;
	snd_pcm_hw_params_t* hwparams;
	snd_pcm_sw_params_t* swparams;

	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t period_size;
	unsigned int nperiod;
	uint8_t* frames;
} PrivInfo;

// generat_sine for single channel
static void generate_sine(uint8_t* frames, int channel, int count, double* _phase) 
{
	double phase = *_phase;
	double max_phase = 1.0 / DEFAULT_SINE_FREQ;
	double step = 1.0 / (double)DEFAULT_RATE;
	double res;
	float fres;
	int    chn = 0;
	int32_t  ires;
	int8_t *samp8 = (int8_t*) frames;
	int16_t *samp16 = (int16_t*) frames;
	int32_t *samp32 = (int32_t*) frames;
	float   *samp_f = (float*) frames;

	while(count -- > 0) {
		//for (chn = 0; chn < DEFAULT_CHANNELS; chn++) {
		//	if (chn == channel) {
		res = (sin((phase * 2 * M_PI) / max_phase - M_PI)) * 0x03fffffff;
		ires = res;
		*samp16++ = ires >> 16;
		//	} else {
		*samp16++ = ires >> 16;
		//	}
		//}

		phase += step;
		if (phase >= max_phase)
			phase -= max_phase;
	}

	*_phase = phase;
}

static int write_buffer(snd_pcm_t* handle, uint8_t* ptr, int cptr) 
{
	int err;

	while(cptr > 0) {
		err = snd_pcm_writei(handle, ptr, cptr);
		
		if (err == -EAGAIN) {
			continue;
		}
		
		if (err < 0) {
			return -1;
		}

		ptr += snd_pcm_frames_to_bytes(handle, err);
		cptr -= err;
	}

	return 0;
}

static int receive_cmd_set_swparam(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);
	int err;

	/* get current swparams */
	err = snd_pcm_sw_params_current(priv->playback_handle, priv->swparams);
	if (err < 0) {
		LOGE("%s: get current swparams error: %s", __func__,
				snd_strerror(err));
		return -1;
	}

	/* start transfer when a buffer is full */
	err = snd_pcm_sw_params_set_start_threshold(priv->playback_handle, 
			priv->swparams, priv->buffer_size);
	if (err < 0) {
		LOGE("%s: snd_pcm_sw_params_set_start_threshold error: %s", __func__,
				snd_strerror(err));
		return -1;
	}

	/* allow the transfer when at least period_size frames can be processed */
	err = snd_pcm_sw_params_set_avail_min(priv->playback_handle,
			priv->swparams, priv->period_size);
	if (err < 0) {
		LOGE("%s: snd_pcm_sw_params_set_avail_min error: %s", __func__, 
				snd_strerror(err));
		return -1;
	}

	/* write the params to playback device */
	err = snd_pcm_sw_params(priv->playback_handle, priv->swparams);
	if (err < 0) {
		LOGE("%s: snd_pcm_sw_params error: %s", __func__, snd_strerror(err));
		return -1;
	}

	return  0;
}

static int receive_cmd_set_hwparam(CmdInterface* thiz)
{
	DECLES_PRIV(priv, thiz);
	unsigned int rrate;
	int err;
	snd_pcm_uframes_t period_size_min;
	snd_pcm_uframes_t period_size_max;
	snd_pcm_uframes_t buffer_size_min;
	snd_pcm_uframes_t buffer_size_max;

	/* chose all params */
	err = snd_pcm_hw_params_any(priv->playback_handle, priv->hwparams);
	if (err < 0) {
		LOGE("%s: chose all params err: %s", __func__, snd_strerror(err));
		return -1;
	}

	/* set inerleaved read/write format */
	err = snd_pcm_hw_params_set_access(priv->playback_handle, priv->hwparams, 
			SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		LOGE("%s: set hwparams access err: %s", __func__, snd_strerror(err));
		return -1;
	}

	/* set the sample rate */
	err = snd_pcm_hw_params_set_format(priv->playback_handle, priv->hwparams,
			SND_PCM_FORMAT_S16_LE);
	if (err < 0) {
		LOGE("%s: set hwparams fomat err: %s", __func__, snd_strerror(err));
		return -1;
	}

	/* set the counts of channels */
	err = snd_pcm_hw_params_set_channels(priv->playback_handle, priv->hwparams, DEFAULT_CHANNELS);
	if (err < 0) {
		LOGE("%s: set hwparams channnels err: %s", __func__, snd_strerror(err));
		return -1;
	}

	/* set stream rate 44100/48000 */
	err = snd_pcm_hw_params_set_rate(priv->playback_handle, priv->hwparams, DEFAULT_RATE, 0);
	if (err < 0) {
		LOGE("%s: set hwparams rate err: %s", __func__, snd_strerror(err));
		return -1;
	}
	//snd_pcm_hw_params_get_rate();
	
	/* set buffer time */
	snd_pcm_hw_params_get_buffer_size_min(priv->hwparams, &buffer_size_min);
	snd_pcm_hw_params_get_buffer_size_max(priv->hwparams, &buffer_size_max);
	snd_pcm_hw_params_get_period_size_min(priv->hwparams, &period_size_min, NULL);
	snd_pcm_hw_params_get_period_size_max(priv->hwparams, &period_size_max, NULL);
	
	priv->buffer_size = (buffer_size_max / DEFAULT_NPERIODS) * DEFAULT_NPERIODS;
	err = snd_pcm_hw_params_set_buffer_size_near(priv->playback_handle,
			priv->hwparams, &priv->buffer_size);
	if (err < 0) {
		LOGE("%s: set params buffer size err: %s", __func__, snd_strerror(err));
		return -1;
	}

	err = snd_pcm_hw_params_set_periods_near(priv->playback_handle, 
			priv->hwparams, &priv->nperiod, NULL);
	if (err < 0) {
		LOGE("%s: set params period size err: %s", __func__, snd_strerror(err));
		return -1;
	}

	err = snd_pcm_hw_params(priv->playback_handle, priv->hwparams);
	if (err < 0) {
		LOGE("%s: set params err: %s", __func__, snd_strerror(err));
		return -1;
	}

	snd_pcm_hw_params_get_buffer_size(priv->hwparams, &priv->buffer_size);
	snd_pcm_hw_params_get_period_size(priv->hwparams, &priv->period_size, NULL);

	LOGE("%s: buffer_size is %lu, period_size is %lu", __func__, 
			priv->buffer_size, priv->period_size);

	return 0;
}

static void* receive_cmd_ring_run(void* ctx)
{
	CmdInterface* thiz = (CmdInterface*)ctx;
	DECLES_PRIV(priv, thiz);

	int n = 0;
	int period = (DEFAULT_RATE*3)/priv->period_size;
	double phase = 0;
	int chn = 0;

	while(priv->thread_run) {
		for (chn = 0; chn < DEFAULT_CHANNELS; chn++) {
			for (n = 0; n < period; n++) {
				//LOGE("%s: generate_sine", __func__);
				generate_sine(priv->frames, chn, priv->period_size, &phase);

				write_buffer(priv->playback_handle, priv->frames, priv->period_size);
			}

			if (priv->buffer_size > n * priv->period_size) {
				snd_pcm_drain(priv->playback_handle);
				snd_pcm_prepare(priv->playback_handle);
			}
		}
	}

	return NULL;
}

static int ring_cmd_execute(CmdInterface* thiz, void* ctx)
{
	DECLES_PRIV(priv, thiz);
	int rt = 0;
	char content[4];
	pthread_t t;
	Parcel* reply = parcel_create();

	parcel_set_main_cmd(reply, DIAG_AUTOTEST_F);
	parcel_set_sub_cmd(reply, AUTOTEST_RING);

	stop_service("media");

	LOGE("%s: alsa_amixer set", __func__);
	system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 1");
	system("alsa_amixer cset -c sprdphone name=\"Earpiece Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Headset Playback Switch\" 0");
	system("alsa_amixer sset \"PCM2\" 80%");

	snd_pcm_hw_params_alloca(&priv->hwparams);
	snd_pcm_sw_params_alloca(&priv->swparams);

	rt = snd_pcm_open(&priv->playback_handle, "plughw:sprdphone",
			SND_PCM_STREAM_PLAYBACK, 0);
	if (rt < 0) {
		LOGE("%s: unable to open SND_PCM_STREAM_PLAYBACK device", __func__);
		return -1;
	}

	receive_cmd_set_hwparam(thiz);
	receive_cmd_set_swparam(thiz);

	priv->frames = malloc(snd_pcm_frames_to_bytes(priv->playback_handle, priv->period_size));

	priv->thread_run = 1;
	pthread_create(&t, NULL, receive_cmd_ring_run, (void*)thiz);

	usleep(200000);
	LOGE("%s: send AUTOTEST_RET_STEP1", __func__);
	memset(content, 0, sizeof(content));
	*(uint8_t*)content = AUTOTEST_RET_STEP1;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);
	usleep(600000);

	/*
	system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Earpiece Playback Switch\" 0");
	system("alsa_amixer cset -c sprdphone name=\"Headset Playback Switch\" 1");

	LOGE("%s: send AUTOTEST_RET_STEP2", __func__);
	// sound change to headset
	memset(content, 0, sizeof(content));
	*(uint8_t*)content = AUTOTEST_RET_STEP2;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);
	usleep(600000); */

	// close all sound
	priv->thread_run = 0;
	pthread_join(t, NULL);

	LOGE("%s: send AUTOTEST_RET_DONE", __func__);
	memset(content, 0, sizeof(content));
	*(uint8_t*)content = AUTOTEST_RET_DONE;
	parcel_set_content(reply, content, 4);
	cmd_listener_reply(priv->listener, reply);

	parcel_destroy(reply);

	SAFE_FREE(priv->frames);
	snd_pcm_close(priv->playback_handle);
	return 0;
}

static void ring_cmd_destroy(CmdInterface* thiz)
{
	SAFE_FREE(thiz);
}

CmdInterface* ring_cmd_create(CmdListener* listener)
{
	CmdInterface* thiz = (CmdInterface*) malloc(sizeof(CmdInterface) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->execute = ring_cmd_execute;
		thiz->destroy = ring_cmd_destroy;

		thiz->cmd = AUTOTEST_RING;
		priv->listener = listener;
		priv->thread_run = 0;
		priv->nperiod = DEFAULT_NPERIODS;
		priv->frames = NULL;
	}

	return thiz;
}
