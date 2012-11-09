#define LOG_TAG "FMNative_jni"

#include "utils/Log.h"
#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"

#include "fmradio.h"

namespace android {

#define CB_MESSAGE_NULL 0;
#define CB_MESSAGE_REGION_TUNE_MODE 1
#define CB_MESSAGE_CHANNEL_FREQ 2
#define CB_MESSAGE_CURRENT_RSSI 3
#define CB_MESSAGE_SEARCH_STEPS 4
#define CB_MESSAGE_MUTE_STATE 5
#define CB_MESSAGE_VOLUME_GAIN 6 
#define CB_MESSAGE_PRESET_CHANNELS 7 
#define CB_MESSAGE_SEARCH_RSSI_THRESHOLD 8 
#define CB_MESSAGE_STEREO_MONO_STATUS 9

#define FM_MESSAGE_SIZE 10
//using namespace android;

struct fields_t {
	jfieldID context;
};

typedef struct fm_context_t {
	JavaVM* vm;
	int envVer;
	jobject me;
}fm_context;

typedef struct fm_message_t {
	int cmd;
	uint8_t param;
}fm_message;

static fm_context* context;
static struct fields_t fields;
static jmethodID method_get_channel_freq_cb;
static jmethodID method_get_current_rssi_cb;
static pthread_t callback_thread;

typedef struct fm_message_buf_t {
	fm_message messages[FM_MESSAGE_SIZE];
	pthread_mutex_t msg_lock;
	int readpos, writepos;
	pthread_cond_t notempty;
	pthread_cond_t notfull;
}fm_message_buf;

void put_message(fm_message_buf* msgs, fm_message message);

static fm_message_buf msgs;

fm_ctrl_block fm_block = {
	24100, // start freq = 88.1MHz
	200,
	100,
	FM_TUNE_AUTOSTEREO | FM_REGION_EUR,	// default region FM_REGION_EUR
	0,	// rssi
	100,	// rssi_threshold
	FM_MUTE_OFF
};

void 
fm_get_region_tune_mode_cb(uint8_t param)
{
	fm_message message;

	fm_block.region_tune_mode = param;
	printf("in cb, region param = %x\n", param);
	switch( param) {
	case 0:
		printf("FM_REGION_EUR | FM_TUNE_MONO\n");
		break;
	case 1:
		printf("FM_REGION_JAN | FM_TUNE_MONO\n");
		break;
	case 2:
		printf("FM_REGION_EUR | FM_TUNE_AUTOSTEREO\n");
		break;
	case 3:
		printf("FM_REGION_JAN | FM_TUNE_AUTOSTEREO\n");
		break;
	}

	message.cmd = CB_MESSAGE_REGION_TUNE_MODE;
	message.param = param;

	put_message(&msgs, message);
}

void
fm_get_channel_freq_cb(uint16_t param)
{
	fm_message message;

	fm_block.freq = param;
	printf("in cb, current freq =%d, %dKHz\n", param, param + 64000);

	message.cmd = CB_MESSAGE_CHANNEL_FREQ;
	message.param = param;
	
	put_message(&msgs, message);
}

void
fm_get_current_rssi_cb(uint8_t param)
{
	fm_message message;

	fm_block.rssi = ~param;
	printf("in cb, current rssi = -%d dBm\n", (uint8_t)~param);

	message.cmd = CB_MESSAGE_CURRENT_RSSI;
	message.param = param;

	put_message(&msgs, message);
}

void
fm_get_search_steps_cb(uint8_t param)
{
	fm_message message;

	fm_block.steps = param;
	printf("in cb, search steps = %dkHz\n", param);
	
	message.cmd = CB_MESSAGE_SEARCH_STEPS;
	message.param = param;

	put_message(&msgs, message);
}

void
fm_get_mute_state_cb(uint8_t param)
{
	fm_message message;

	fm_block.mute_state = param;
	if (param == FM_MUTE_OFF)
		printf("in cb, mute_state = mute off\n");
	else
		printf("in cb, mute_state = mute on\n");

	message.cmd = CB_MESSAGE_MUTE_STATE;
	message.param = param;

	put_message(&msgs, message);
}

void
fm_get_volume_gain_cb(uint16_t param)
{
	fm_message message;

	fm_block.volume = param;
	printf("in cb, volume gain = %d\n", param);

	message.cmd = CB_MESSAGE_VOLUME_GAIN;
	message.param = param;

	put_message(&msgs, message);
}

void
fm_get_preset_channels_cb(uint8_t plen, uint8_t *param)
{
	fm_message message;

	uint16_t channel;
	uint8_t *p;
	int i;

	printf("in cb, preset_channels=%d\n", plen>>2);

	p = param;
	i = plen>>2;
	while (i>0) {
		channel = ((*(p+1))<<8) + (*p);
		if (channel == 0)
			break;
		printf("%d KHz\n", channel + 64000);
		p +=4;
		i--;
	}

	message.cmd = CB_MESSAGE_PRESET_CHANNELS;
	message.param = *param;

	put_message(&msgs, message);
}

void
fm_get_search_rssi_threshold_cb(uint8_t param)
{
	fm_message message;

	printf("in cb, rssi_threshold = -%ddBm\n", param);

	message.cmd = CB_MESSAGE_SEARCH_RSSI_THRESHOLD;
	message.param = param;

	put_message(&msgs, message);
}

void 
fm_get_stereo_mono_status_cb(uint8_t param)
{
	fm_message message;

	if (param == FM_AUDIO_STEREO)
		printf("in cb, audio STEREO\n");
	else
		printf("in cb, audio MONO\n");

	message.cmd = CB_MESSAGE_STEREO_MONO_STATUS;
	message.param = param;

	put_message(&msgs, message);
}

fm_callbacks fm_cbs = {
	&fm_get_region_tune_mode_cb,
	&fm_get_channel_freq_cb,
	&fm_get_current_rssi_cb,
	&fm_get_search_steps_cb,
	&fm_get_mute_state_cb,
	&fm_get_volume_gain_cb,
	&fm_get_preset_channels_cb,
	&fm_get_search_rssi_threshold_cb,
	&fm_get_stereo_mono_status_cb
};

void
cmd_fmon(const char *cmdparam)
{
	fm_func_on(&fm_block);
}
void
cmd_fmoff(const char *cmdparam)
{
	fm_func_off();
}
void
cmd_tunefreq(const char *cmdparam)
{
	uint32_t freq;
	
	sscanf(cmdparam, "%d", &freq);
	freq -= 64000;
	fm_tune_freq((uint16_t)freq);
}
void
cmd_getfreq(const char *cmdparam)
{
	fm_get_current_freq();
}

void
cmd_setthresh(const char *cmdparam)
{
	uint16_t thresh;
	
	sscanf(cmdparam, "%hu", &thresh);
	fm_block.rssi_thresh = thresh;
	fm_set_search_rssi_threshold((uint8_t)thresh);
	fm_get_search_rssi_threshold();
}

void
cmd_getthresh(const char *cmdparam)
{
	fm_get_search_rssi_threshold();
}

void
cmd_getrssi(const char *cmdparam)
{
	fm_get_current_rssi();
}

void
cmd_searchup(const char *cmdparam)
{
	uint32_t freq;
	
	sscanf(cmdparam, "%d", &freq);
	freq -= 64000;
	if (freq > 44000)
		freq = fm_block.freq +100;

	fm_search(FM_SEARCH_UP, (uint16_t)freq);
}

void
cmd_searchdown(const char *cmdparam)
{
	uint32_t freq;
	
	sscanf(cmdparam, "%d", &freq);
	freq -= 64000;
	if (freq > 44000)
		freq = fm_block.freq -100;
	fm_search(FM_SEARCH_DOWN, (uint16_t)freq);
}

void
cmd_autoup(const char *cmdparam)
{
	uint32_t freq;
	uint16_t channels;
	
	sscanf(cmdparam, "%hu %d", &channels, &freq);
	freq -= 64000;
	if (freq >44000)
		freq = fm_block.freq + 100; 
	fm_auto_search(FM_SEARCH_UP, (uint16_t)freq, (uint8_t)channels);
}


void
cmd_autodown(const char *cmdparam)
{
	uint32_t freq;
	uint16_t channels;
	
	sscanf(cmdparam, "%hu %d", &channels, &freq);
	freq -= 64000;
	if (freq > 44000)
		freq = fm_block.freq - 100; 
	fm_auto_search(FM_SEARCH_DOWN, (uint16_t)freq, (uint8_t)channels);
}

void
cmd_setsteps50(const char *cmdparam)
{
	fm_set_search_steps(FM_SEARCH_STEPS_50KHz);
	fm_get_search_steps();
}

void
cmd_setsteps100(const char *cmdparam)
{
	fm_set_search_steps(FM_SEARCH_STEPS_100KHz);
	fm_get_search_steps();
}

void
cmd_getsteps(const char *cmdparam)
{
	fm_get_search_steps();
}

void
cmd_mono(const char *cmdparam)
{
	fm_set_region_tune_mode(fm_block.region_tune_mode & 0x1);
	fm_get_region_tune_mode();
}

void
cmd_stereo(const char *param)
{
	fm_set_region_tune_mode((fm_block.region_tune_mode & 0x1)|0x2);
	fm_get_region_tune_mode();
}

void
cmd_setvol(const char *cmdparam)
{
	uint16_t vol;
	
	sscanf(cmdparam, "%hu", &vol);
	fm_set_volume(vol);
}

void
cmd_getvol(const char *cmdparam)
{
	fm_get_volume();
}

void
cmd_mute(const char *cmdparam)
{
	fm_mute();
}

void
cmd_unmute(const char *cmdparam)
{
	fm_unmute();
}

void
cmd_getmute(const char *cmdparam)
{
	fm_get_mute_state();
}

void
cmd_abort(const char *cmdparam)
{
	fm_searchabort();
}

void
cmd_getaudiostatus(const char *cmdparam)
{
	fm_get_stereo_mono_status();
}

static struct {
	const char *cmd;
	void (*func)(const char *cmdparam);
	char *doc;
} command[] = {
	{"on", cmd_fmon, "FM power on"},
	{"off", cmd_fmoff, "FM power off"},
	{"freq", cmd_tunefreq, "Tune to a specific freq, param <freq>"},
	{"getfreq", cmd_getfreq, "Get current freq"},
	{"thresh", cmd_setthresh, "Set search rssi threshold, param <rssi>"},
	{"getth", cmd_getthresh, "Get search rssi threshold"},
	{"rssi", cmd_getrssi, "Get current rssi"},
	{"up", cmd_searchup, "Search UP, param [freq]"},
	{"down", cmd_searchdown, "Search DOWN, param [freq]"},
	{"aup", cmd_autoup, "Auto search UP, param <channels> [freq]"},
	{"adown", cmd_autodown, "Auto search DOWN, param <channels> [freq]"},
	{"abort", cmd_abort, "Abort search"},
	{"s50", cmd_setsteps50, "Set search steps 50 KHz"},
	{"s100", cmd_setsteps100, "Set search steps 100 KHz"},
	{"getstep", cmd_getsteps, "Get search steps"},
	{"mono", cmd_mono, "Set mono mode"},
	{"stereo", cmd_stereo, "Set stereo mode"},
	{"setvol", cmd_setvol, "Set FM volume, param <volume>"},
	{"getvol", cmd_getvol, "Get FM volume"},
	{"mute", cmd_mute, "FM mute"},
	{"unmute", cmd_unmute, "FM unmute"},
	{"getmute", cmd_getmute, "Get mute state"},
	{"aust", cmd_getaudiostatus, "Get current stereo/mono status"},
	{"exit", NULL, "Exit this application"},
	{NULL, NULL, NULL},
};

fm_message get_message(fm_message_buf* msgs) 
{
	fm_message message;

	pthread_mutex_lock(&msgs->msg_lock);
	
	if(msgs->writepos == msgs->readpos)
	{
		pthread_cond_wait(&msgs->notempty, &msgs->msg_lock);
	}

	message = msgs->messages[msgs->readpos];
	msgs->readpos++;

	if(msgs->readpos >= FM_MESSAGE_SIZE) {
		msgs->readpos = 0;
	}

	pthread_cond_signal(&msgs->notfull);
	pthread_mutex_unlock(&msgs->msg_lock);
	return message;
}

void put_message(fm_message_buf* msgs, fm_message message)
{
	pthread_mutex_lock(&msgs->msg_lock);

	if ((msgs->writepos + 1) % FM_MESSAGE_SIZE == msgs->readpos)
	{
		pthread_cond_wait(&msgs->notfull, &msgs->msg_lock);
	}

	msgs->messages[msgs->writepos] = message;
	msgs->writepos++;

	if(msgs->writepos >= FM_MESSAGE_SIZE)
	{
		msgs->writepos = 0;
	}

	pthread_cond_signal(&msgs->notempty);
	pthread_mutex_unlock(&msgs->msg_lock);
}

void *fm_cb_thread(void* arg)
{
	fm_message message;
	fm_context* context = (fm_context*)arg;
	JNIEnv* env;
	JavaVMAttachArgs args;
	char name[] = "FM_Thread";
	args.version = context->envVer;
	args.name = name;
	args.group = NULL;

	context->vm->AttachCurrentThread(&env, &args);
//	context->vm->GetEnv((void**)&env, context->envVer);

	while(1) {
		message = get_message(&msgs);
		switch(message.cmd) {
			case CB_MESSAGE_REGION_TUNE_MODE:
				break;
			case CB_MESSAGE_CHANNEL_FREQ:
				break;
			case CB_MESSAGE_CURRENT_RSSI:
				break;
			case CB_MESSAGE_SEARCH_STEPS:
				break;
			case CB_MESSAGE_MUTE_STATE:
				break;
			case CB_MESSAGE_VOLUME_GAIN:
				break;
			case CB_MESSAGE_PRESET_CHANNELS:
				break;
			case CB_MESSAGE_SEARCH_RSSI_THRESHOLD:
				break;
			case CB_MESSAGE_STEREO_MONO_STATUS:
				break;
		}
	}

	return NULL;
}

void init_msgs(fm_message_buf* msgs)
{
	pthread_mutex_init(&msgs->msg_lock, NULL);
	pthread_cond_init(&msgs->notempty, NULL);
	pthread_cond_init(&msgs->notfull, NULL);
	msgs->readpos = 0;
	msgs->writepos = 0;
}

static void com_broncho_jnibt_FMNative_native_init(JNIEnv* env, jobject object) 
{
	LOGE(__FUNCTION__);
	jclass clazz;
	int ret;
	
	context = (fm_context*) calloc(1, sizeof(fm_context));
	if(NULL == context) {
		LOGE("%s: out of memory-fm_context!", __FUNCTION__);
		return;
	}

	clazz = env->GetObjectClass(object);
	if(clazz == NULL) {
		LOGE("Cannot find FMNative.");
		jniThrowException(env, "java/lang/RuntimeException", "Cannot find com/broncho/jnibt/FMNative");
		return;
	}

	method_get_channel_freq_cb = env->GetMethodID(clazz, "get_channel_freq_cb", "(F)V");
	method_get_current_rssi_cb = env->GetMethodID(clazz, "get_current_rssi_cb", "(F)V");

	fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
	if (fields.context == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Cannot find FMNative.mNativeContext");
		return;
	}

	env->SetIntField(object, fields.context, (jint)context);
	env->GetJavaVM(&(context->vm));
	context->envVer = env->GetVersion();
	context->me = env->NewGlobalRef(object);

	//init msgs
	init_msgs(&msgs);

	//start callback thread
	pthread_create(&callback_thread, NULL, fm_cb_thread, (void*)context);
	//register callback here!
	ret = fm_init(&fm_cbs);
	if (ret != 0) {
		LOGE("fm_init failed.\n");
		return;
	}

	ret = fm_func_on(&fm_block);
	if (ret != 0) {
		LOGE("fm_func_on failed.\n");
		return;
	}
}

static void com_broncho_jnibt_FMNative_send_cmd(JNIEnv* env, jobject object, jstring cmd, jstring cmdparam)
{
	int i;

	if (cmd == NULL) {
		jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
		return;
	}

	const char* cmdStr = env->GetStringUTFChars(cmd, NULL);
	if (cmdStr == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
		return;
	}

	const char* paramStr = env->GetStringUTFChars(cmdparam, NULL);
	if (paramStr == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
		return;
	}

	for (i=0; command[i].cmd; i++)
	{
		if (strncmp(command[i].cmd, cmdStr, strlen(command[i].cmd))) {
			if (command[i+1].cmd == NULL) {
				LOGE("unknown cmd");
			}
			continue;
		}

		if (strncmp(command[i].cmd, "exit", strlen("exit"))) {
			command[i].func(paramStr);
			break;
		} else {
			fm_func_off();
			fm_close();
			return;
		}

	}
}

/*
static void com_broncho_jnibt_FMNative_class_init(JNIEnv* env, jclass clazz)
{
	method_get_channel_freq_cb = env->GetMethodID(clazz, "get_channel_freq_cb", "(F)V");
	method_get_current_rssi_cb = env->GetMethodID(clazz, "get_current_rssi_cb", "(F)V");
}*/

static JNINativeMethod gMethods[] = {
//	{"class_init", "()V", (void*)com_broncho_jnibt_FMNative_class_init},
	{"native_init", "()V", (void*)com_broncho_jnibt_FMNative_native_init},
	{"send_cmd", "(Ljava/lang/String;Ljava/lang/String)V", (void*)com_broncho_jnibt_FMNative_send_cmd},
};

static const char* const KClassPathName = "com/broncho/jnibt/FMNative";

int register_com_broncho_jnibt_FMNative(JNIEnv* env) 
{
	return AndroidRuntime::registerNativeMethods(env, 
			"com/broncho/jnibt/FMNative", gMethods, NELEM(gMethods));
}
}
