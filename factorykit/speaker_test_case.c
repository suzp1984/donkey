#include "speaker_test_case.h"
#include "ui.h"
#include "minui.h"
#include "common.h"
#include "eng_wav.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>

#define SPRD_AUDIO_FILE             "/data/eng.wav"

typedef struct {
	int thread_run;
} PrivInfo;

static void speaker_test_create_wav(TestCase* thiz)
{
	int fd;
	FILE* fp;

	fd = open(SPRD_AUDIO_FILE, O_CREAT | O_RDWR);
	if (fd < 0) {
		LOGE("%s: open %s fail", __func__, SPRD_AUDIO_FILE);
		return;
	}

	fp = fdopen(fd, "wb");
	if (fwrite(wav_data, sizeof(unsigned char), SOUND_LENGTH, fp) != SOUND_LENGTH) {
		LOGE("%s: fwrite wav data fail", __func__);
		return;
	}

	if (fp != NULL) {
		fclose(fp);
	}

	if (fd > 0) {
		close(fd);
	}

}

static void* speaker_test_play(void* ctx)
{
	TestCase* thiz = (TestCase*)ctx;
	DECLES_PRIV(priv, thiz);
	char cmd[128];
	int status;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%s %s", "alsa_aplay -Dplughw:sprdphone", SPRD_AUDIO_FILE);

	while(priv->thread_run == 1) {
		system("alsa_amixer sset \"PCM\" 100%");
		system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 1");
		//set_audio_mode(0x2);
		status = system(cmd);
		if (status < 0) {
			LOGE("%s: Error[%d] [%s]", __func__, status, strerror(errno));
			break;
		} else {
			LOGE("%s: Fine ", __func__);
		}

		system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
		system("alsa_amixer -c sprdphone cset name='Power Codec' 4");
	}

	return NULL;
}

static int speaker_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	ui_screen_clean();
	pthread_t t;

	gr_color(255, 255, 255, 255);
	ui_draw_title(UI_TITLE_START_Y, SPEAKER_TEST_CASE);
	gr_flip();

	fk_stop_service("media");

	speaker_test_create_wav(thiz);
	priv->thread_run = 1;

	pthread_create(&t, NULL, speaker_test_play, (void*)thiz);
	thiz->passed = ui_draw_handle_softkey(thiz->name);
	priv->thread_run = 0;
	pthread_join(t, NULL);

	return 0;
}

static void speaker_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* speaker_test_case_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = speaker_test_run;
		thiz->destroy = speaker_test_destroy;

		thiz->name = SPEAKER_TEST_CASE;
		thiz->passed = passed;
		priv->thread_run = 0;
	}

	return thiz;
}
