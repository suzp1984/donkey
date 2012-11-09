#include <arch/linux-arm/AndroidConfig.h>
#include <stdint.h>
#include <sys/types.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#define HAVE_SYS_UIO_H
#define LOG_NDEBUG 0
#define LOG_TAG "AudioHardwareLittleton"
#include <utils/Log.h>
#include <utils/String8.h>
#include <media/AudioSystem.h>
#include <media/AudioRecord.h>

#include "AudioHardwareMarvell.h"
#include "AudioProtocol.h"

namespace android {

static char const * const sRecord = "hw:0,1";
static char const * const sHIFI = "hw:0,0,0";
static char const * const sVoice = "hw:0,1,0";
static char const * const sPhone = "hw:0,2,0";
static char const * const sBluetooth = "bluetooth";

//---------------Audio related functions--------------------
void enable_hifi_mono(unsigned char vol)
{
	unsigned char rev = (vol * 63) / 100;
	char formats[256];
	rev = 63 - rev;
	
    system("amixer cset iface='MIXER',name='MONO Mux' 3 'DAC1'");
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Mono Volume' ",rev);
    system(formats);	
}

void disable_hifi_mono()
{
    system("amixer cset iface='MIXER',name='MONO Mux' 0 'AUX1'");
    system("amixer cset iface='MIXER',name='Mono Volume' 63");
}

void set_hifi_mono_volume(unsigned char vol)
{
	unsigned char rev = (vol * 63) / 100;
	char formats[256];
	rev = 63 - rev;

    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Mono Volume' ",rev);
    system(formats);	
}

void enable_hifi_bear(unsigned char vol)
{
	unsigned char rev = (vol * 63) / 100;
	char formats[256];
	rev = 63 - rev;
	
    system("amixer cset iface='MIXER',name='BEAR Mux' 3 'DAC1'");
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Bear Volume' ",rev);
    system(formats);	
}

void disable_hifi_bear()
{
    system("amixer cset iface='MIXER',name='BEAR Mux' 0 'AUX1'");
    system("amixer cset iface='MIXER',name='Bear Volume' 63");
}

void set_hifi_bear_volume(unsigned char vol)
{
	unsigned char rev = (vol * 63) / 100;
	char formats[256];
	rev = 63 - rev;
	
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Bear Volume' ",rev);
    system(formats);	
}

void enable_hifi_headset(unsigned char vol)
{
	unsigned char rev = (vol * 63) / 100;
	char formats[256];
	rev = 63 - rev;
	
    system("amixer cset iface='MIXER',name='STEREO_CH1 Mux' 3 'DAC1'");
    system("amixer cset iface='MIXER',name='STEREO_CH2 Mux' 4 'DAC2'");
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Stereo Ch1 Volume' ",rev);
    system(formats);	
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Stereo Ch2 Volume' ",rev);
    system(formats);	    
}

void disable_hifi_headset()
{
    system("amixer cset iface='MIXER',name='STEREO_CH1 Mux' 0 'AUX1'");
    system("amixer cset iface='MIXER',name='STEREO_CH2 Mux' 0 'AUX1'");
    system("amixer cset iface='MIXER',name='Stereo Ch1 Volume' 63");
    system("amixer cset iface='MIXER',name='Stereo Ch2 Volume' 63");    
}

void set_hifi_headset_volume(unsigned char vol)
{
	unsigned char rev = (vol * 63) / 100;
	char formats[256];
	rev = 63 - rev;
	
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Stereo Ch1 Volume' ",rev);
    system(formats);	
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Stereo Ch2 Volume' ",rev);
    system(formats);	    
}

void enable_voice_mic1(unsigned char vol)
{
	unsigned char rev = (vol * 7) / 100;
	char formats[256];
	rev = 7 - rev;
	
    system("amixer cset iface='MIXER',name='TX Mux' 2 'Mic1'");
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Tx Volume' ",rev);
    system(formats);	
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Mic Volume' ",rev);
    system(formats);	    
}

void disable_voice_mic1()
{
    system("amixer cset iface='MIXER',name='TX Mux' 0 'AUX2'");
    system("amixer cset iface='MIXER',name='Tx Volume' 7");
    system("amixer cset iface='MIXER',name='Mic Volume' 7");    
}

void set_voice_mic1_volume(unsigned char vol)
{
	unsigned char rev = (vol * 7) / 100;
	char formats[256];
	rev = 7 - rev;
	
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Tx Volume' ",rev);
    system(formats);	
    sprintf(formats,"%s%d","amixer cset iface='MIXER',name='Mic Volume' ",rev);
    system(formats);	    
}

// ----------------------------------------------------------------------------

AudioHardwareMarvell::AudioHardwareMarvell() : mOutput(0), mInput(0), mMicMute(false)
{
    sem_init(&mLock, 0, 1);
    mOutputHandle = NULL;
    mInputHandle = NULL;
    mMode = AudioSystem::MODE_INVALID;
    mVoiceVolume = 0.75f;
}

AudioHardwareMarvell::~AudioHardwareMarvell()
{
    if(mOutput)
        delete mOutput;
    if(mInput)
        delete mInput;
    sem_destroy(&mLock);
}

status_t AudioHardwareMarvell::initCheck()
{
    LOGI("*************************AudioHardwareMarvell::initCheck***************************\n");
    return NO_ERROR;
}

status_t AudioHardwareMarvell::standby()
{
    return NO_ERROR;
}

AudioStreamOut* AudioHardwareMarvell::openOutputStream(
        int format, int channelCount, uint32_t sampleRate, status_t *status)
{
    // only one output stream allowed
    *status = NO_ERROR;
    if (mOutput) 
        return NULL;

    LOGI("*************************AudioHardwareMarvell::openOutputStream***************************\n");
    // create new output stream
    AudioStreamOutMarvell* out = new AudioStreamOutMarvell();
    mOutputFormat = format;
    mOutputChannelCount = channelCount;
    mOutputSampleRate = sampleRate;	
    mOutput = out;
    return mOutput;
}

void AudioHardwareMarvell::closeOutputStream(AudioStreamOutMarvell* out) {
    if (out == mOutput) {
        mOutput = NULL;
        if(mOutputHandle) {
            snd_pcm_close(mOutputHandle);
            mOutputHandle = NULL;
        }
	    switch(mMode)
	    {
	    case AudioSystem::MODE_NORMAL:
	    case AudioSystem::MODE_RINGTONE:
			    switch(mRoutes[mMode])
			    {
			    case AudioSystem::ROUTE_EARPIECE:
			        disable_hifi_bear();
			        break;
			    case AudioSystem::ROUTE_SPEAKER:
			        disable_hifi_mono();
			        break;
			    case AudioSystem::ROUTE_BLUETOOTH_SCO:
			        /* Not support yet */
			        break;
			    case AudioSystem::ROUTE_HEADSET:
			        disable_hifi_headset();
			        break;
			    default:
			        break;
			    };    	
	        break;
	    case AudioSystem::MODE_IN_CALL:
	        /* Not support yet */
	        break;
	    default:
	        break;
	    }
    }
}

AudioStreamIn* AudioHardwareMarvell::openInputStream(
        int inputSource, int format, int channelCount, uint32_t sampleRate, status_t *status, AudioSystem::audio_in_acoustics acoustics)
{
    // check for valid input source
    if ((inputSource < AudioRecord::DEFAULT_INPUT) ||
        (inputSource >= AudioRecord::NUM_INPUT_SOURCES)) {
        return 0;
    }

    // only one input stream allowed
    *status = NO_ERROR;
    if (mInput) 
        return NULL;

    LOGI("*************************AudioHardwareMarvell::openInputStream***************************\n");
    // create new input stream
    AudioStreamInMarvell* in = new AudioStreamInMarvell();
    mInputFormat = format;
    mInputChannelCount = channelCount;
    mInputSampleRate = sampleRate;
    int ret = snd_pcm_open(&mInputHandle, sRecord, SND_PCM_STREAM_CAPTURE, 0);
    if (ret < 0) {
        LOGE("***************snd_pcm_open error: %s**********************\n", snd_strerror(ret));
        delete in;
        return NULL;
    }
    in->setInputHandle(mInputHandle);
    if (in->set(this, mInputFormat, mInputChannelCount, mInputSampleRate, acoustics) != NO_ERROR) {
        snd_pcm_close(mInputHandle);
        mInputHandle = NULL;
        in->setInputHandle(mInputHandle);
        delete in;
        return NULL;
    }

    unsigned char nv = mVoiceVolume * 100;
    enable_voice_mic1(nv);

    mInput = in;
    return mInput;	
}

void AudioHardwareMarvell::closeInputStream(AudioStreamInMarvell* in) {
    if (in == mInput) {
        mInput = NULL;
        if(mInputHandle) {
            snd_pcm_close(mInputHandle);
            mInputHandle = NULL;
        }
        disable_voice_mic1();
    }
}

void AudioHardwareMarvell::setVolume(int direction, float volume) 
{ 
    LOGI("*************************AudioHardwareMarvell::setVolume(%d, %f)***************************\n", direction, volume);
    unsigned char nv = volume * 100; 
    if(direction == INPUT) {
    	set_voice_mic1_volume(nv);
    } else {
	    switch(mMode)
	    {
	    case AudioSystem::MODE_NORMAL:
	    case AudioSystem::MODE_RINGTONE:
			    switch(mRoutes[mMode])
			    {
			    case AudioSystem::ROUTE_EARPIECE:
			        set_hifi_bear_volume(nv);
			        break;
			    case AudioSystem::ROUTE_SPEAKER:
			        set_hifi_mono_volume(nv);
			        break;
			    case AudioSystem::ROUTE_BLUETOOTH_SCO:
			        /* Not support yet */
			        break;
			    case AudioSystem::ROUTE_HEADSET:
			        set_hifi_headset_volume(nv);
			        break;
			    default:
			        break;
			    };    	
	        break;
	    case AudioSystem::MODE_IN_CALL:
	        /* Not support yet */
	        break;
	    default:
	        break;
	    }    	
    }
}

status_t AudioHardwareMarvell::setRouting(int mode, uint32_t routes)
{
    LOGI("*************************AudioHardwareMarvell::setRouting(%d,%d)***************************\n", mode, routes);
    if (mode == AudioSystem::MODE_CURRENT)
        mode = mMode;
    if ((mode < 0) || (mode >= AudioSystem::NUM_MODES))
        return BAD_VALUE;
    /* Currently littleton does not support tel */
    if (mode == AudioSystem::MODE_IN_CALL)
        return BAD_VALUE;
    uint32_t old = mRoutes[mode];
    mRoutes[mode] = routes;
    if ((mode != mMode) || (old == routes))
        return NO_ERROR;
    mOldRouting = old;	
    mOldMode = mMode;	
    return doRouting();
}

status_t AudioHardwareMarvell::getRouting(int mode, uint32_t* routes)
{
    LOGI("*************************AudioHardwareMarvell::getRouting(%d)***************************\n", mode);
    if (mode == AudioSystem::MODE_CURRENT)
        mode = mMode;
    if ((mode < 0) || (mode >= AudioSystem::NUM_MODES))
        return BAD_VALUE;
    *routes = mRoutes[mode];
    return NO_ERROR;
}

status_t AudioHardwareMarvell::setMode(int mode)
{
    LOGI("*************************AudioHardwareMarvell::setMode(%d)***************************\n", mode);
    bool needAction = false;
    if ((mode < 0) || (mode >= AudioSystem::NUM_MODES))
        return BAD_VALUE;
    if (mMode == mode)
        return NO_ERROR;
    /* Currently littleton does not support tel */
    if (mode == AudioSystem::MODE_IN_CALL)
        return BAD_VALUE;        
    /* Under these two conditions, we must take action.
     * 1: Change to or from Call Mode;
     * 2: Mode is not invalid before;
     */
    if((AudioSystem::MODE_IN_CALL == mode) 
		|| (AudioSystem::MODE_IN_CALL == mMode) 
		|| (AudioSystem::MODE_INVALID == mMode))
        needAction = true;
    mOldMode = mMode;
    if(mOldMode > AudioSystem::MODE_INVALID)
        mOldRouting = mRoutes[mOldMode];	
    else
        mOldRouting = 0;
    mMode = mode;
    if(needAction)
        return doRouting();
    else
        return NO_ERROR;
}

status_t AudioHardwareMarvell::getMode(int *mode)
{
	LOGI("*************************AudioHardwareMarvell::getMode()***************************\n");
	*mode = mMode;
	return NO_ERROR;
}

status_t AudioHardwareMarvell::disableAlsaDevice()
{
    if(mOutput) {
	    mOutput->setOutputHandle(NULL);
	    if(mOutputHandle) {
	        //snd_pcm_drain(mOutputHandle);
	        snd_pcm_close(mOutputHandle);
	        mOutputHandle = NULL;	
	    }           
    }
    return NO_ERROR;
}


status_t AudioHardwareMarvell::enableAlsaDevice(const char* name)
{
    LOGI("*************************enableAlsaDevice(%s)***************************\n",name);
    disableAlsaDevice();

    if(mOutput) {           
	    int ret = snd_pcm_open(&mOutputHandle, name, SND_PCM_STREAM_PLAYBACK, 0);
	    if (ret < 0) {
	        return BAD_VALUE;
	    }
	    mOutput->setOutputHandle(mOutputHandle);
	    if (mOutput->set(this, mOutputFormat, mOutputChannelCount, mOutputSampleRate) != NO_ERROR) {
	        snd_pcm_close(mOutputHandle);
	        mOutputHandle = NULL;
                mOutput->setOutputHandle(mOutputHandle);	
                return BAD_VALUE;		
	    }
    }
    return NO_ERROR;
}

status_t AudioHardwareMarvell::doRouting()
{
    unsigned char nv = mVoiceVolume * 100;

    LOGI("*************************AudioHardwareMarvell::doRouting(%d, %d, %d, %d)***************************\n",mOldMode,mOldRouting,mMode,mRoutes[mMode]);	
    // Disable previous audio path
    switch(mOldMode)
    {
    case AudioSystem::MODE_NORMAL:
    case AudioSystem::MODE_RINGTONE:
		    switch(mOldRouting)
		    {
		    case AudioSystem::ROUTE_EARPIECE:
		        disable_hifi_bear();
		        break;
		    case AudioSystem::ROUTE_SPEAKER:
		        disable_hifi_mono();
		        break;
		    case AudioSystem::ROUTE_BLUETOOTH_SCO:
		        /* Not support yet */
		        break;
		    case AudioSystem::ROUTE_HEADSET:
		        disable_hifi_headset();
		        break;
		    default:
		        break;
		    };    	
        break;
    case AudioSystem::MODE_IN_CALL:
        /* Not support yet */
        break;
    default:
        break;
    }    	    

    // Enable new audio path and open new ALSA device
    switch(mMode)
    {
    case AudioSystem::MODE_NORMAL:
    case AudioSystem::MODE_RINGTONE:		
        switch(mRoutes[mMode])
        {
        case AudioSystem::ROUTE_EARPIECE:
            enable_hifi_bear(nv);
            enableAlsaDevice(sHIFI);			
            break;
        case AudioSystem::ROUTE_SPEAKER:
            enable_hifi_mono(nv);
            enableAlsaDevice(sHIFI);
            break;
        case AudioSystem::ROUTE_BLUETOOTH_SCO:			
            /* Not support yet */			
            break;
        case AudioSystem::ROUTE_HEADSET:
            enable_hifi_headset(nv);
            enableAlsaDevice(sHIFI);
            break;
        default:
            break;
        };
        break;

    case AudioSystem::MODE_IN_CALL:
        /* Not support yet */
        break;
    };
    return NO_ERROR;
}

status_t AudioHardwareMarvell::setVoiceVolume(float volume)
{
	mVoiceVolume = volume;
    return NO_ERROR;
}

status_t AudioHardwareMarvell::setMasterVolume(float volume)
{
    // return error - software mixer will handle it
    return INVALID_OPERATION;
}

status_t AudioHardwareMarvell::setMicMute(bool state) 
{
    LOGI("*************************AudioHardwareMarvell::setMicMute(%d)***************************\n",state); 
    mMicMute = state;  
    unsigned char nv = 0;
    if(!mMicMute) {
    	nv = mVoiceVolume * 100;
    	set_voice_mic1_volume(nv);
    }
    set_voice_mic1_volume(nv);
    return  NO_ERROR; 
}

status_t AudioHardwareMarvell::getMicMute(bool* state) 
{ 
    *state = mMicMute ; 
    return NO_ERROR; 
}

status_t AudioHardwareMarvell::setParameter(const char* key, const char* value)
{ 
    return NO_ERROR; 
}

status_t AudioHardwareMarvell::dumpInternals(int fd, const Vector<String16>& args)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    result.append("AudioHardwareMarvell::dumpInternals\n");
    snprintf(buffer, SIZE, "\tmMicMute: %s\n", mMicMute? "true": "false");
    result.append(buffer);
    ::write(fd, result.string(), result.size());
    return NO_ERROR;
}

status_t AudioHardwareMarvell::dump(int fd, const Vector<String16>& args)
{
    dumpInternals(fd, args);
    return NO_ERROR;
}

// ----------------------------------------------------------------------------
/*
 *   Underrun and suspend recovery
 */
static int xrun_recovery (snd_pcm_t * handle, int err)
{
    if (err == -EPIPE) {          /* under-run */
        err = snd_pcm_prepare (handle);
        if (err < 0) {
            LOGE("Can't recovery from underrun, prepare failed: %s", snd_strerror (err));
            return err;
        }
        return 0;
    } else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume (handle)) == -EAGAIN)
            usleep (100);           /* wait until the suspend flag is released */

        if (err < 0) {
            err = snd_pcm_prepare (handle);
            if (err < 0) {
                LOGE("Can't recovery from suspend, prepare failed: %s", snd_strerror (err));
                return err;
            }
        }
        return 0;
    }
    return err;
}

// ----------------------------------------------------------------------------

AudioStreamOutMarvell::AudioStreamOutMarvell() 
{
    sem_init(&mLock, 0, 1);
    mFormat = AudioSystem::PCM_16_BIT;
    mSampleRate = 48000;
    mChannelCount = 2;
    buffer_time = 100000;
    period_time = buffer_time/4;
    mAudioHardware = NULL;
}

AudioStreamOutMarvell::~AudioStreamOutMarvell()
{
    if (mAudioHardware)
        mAudioHardware->closeOutputStream(this);
    sem_destroy(&mLock);
}

status_t AudioStreamOutMarvell::set(AudioHardwareMarvell *hw, int format, int channels, uint32_t rate)
{
    // fix up defaults
    if (format == 0) format = mFormat;
    if (channels == 0) channels = channelCount();
    if (rate ==0) rate = sampleRate();

    int requestedRate = rate;
    unsigned int requestedBufferTime = buffer_time;
    unsigned int requestedPeriodTime = period_time;
    unsigned int temp;

    // check values
    if (format != AudioSystem::PCM_16_BIT)
        return BAD_VALUE;

    snd_pcm_hw_params_t *params = NULL;
    snd_pcm_hw_params_alloca(&params);
    
    snd_pcm_hw_params_any(mHandle, params);

    /* Set the desired hardware parameters. */
    snd_pcm_hw_params_set_access(mHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(mHandle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(mHandle, params, channels);

    snd_pcm_hw_params_set_rate_near(mHandle, params, &rate, NULL);
    if( requestedRate != rate ){
	LOGW("unable to set requested rate as %d", requestedRate);
    }
    LOGV("rate eventually set as %d", rate);

    if( snd_pcm_hw_params_set_rate_resample(mHandle, params, 0)<0 ){
	LOGW("unable to disable hw resampling");
    }
    
    //snd_pcm_hw_params_get_buffer_time(params, &temp, 0);
    //LOGI("original buffer time is %d usec", temp);
    //snd_pcm_hw_params_get_period_time_max(params, &temp, 0);
    //LOGI("maximum period time is %d usec", temp);

    if( snd_pcm_hw_params_set_buffer_time_near(mHandle, params, &buffer_time, 0)<0 ){
	LOGW("Unable to set buffer time to %d usec", requestedBufferTime);
    }
    LOGV("buffer time eventually set as %d usec", buffer_time);      
 
    if( snd_pcm_hw_params_set_period_time_near(mHandle,params, &period_time, 0)<0 ){
	LOGW("Unable to set period time to %u usec", requestedPeriodTime);
    }
    LOGV("period time eventually set as %d usec", period_time);
    //snd_pcm_hw_params_get_period_time_max(params, &temp, 0);
    //LOGI("maximum period time is %d usec", temp);

    /* Write the parameters to the driver */
    int ret = snd_pcm_hw_params(mHandle, params);
    if (ret < 0) {
        LOGE("***************unable to set hw parameters: %s**********************\n", snd_strerror(ret));
        return BAD_VALUE;
    }

    /* soft params */
    snd_pcm_sw_params_t *sw_params = NULL;

    if(snd_pcm_sw_params_malloc(&sw_params) < 0) {
	LOGE("Failed to allocate ALSA software parameters!");
	return BAD_VALUE;
    }

    // Get the current software parameters
    if(snd_pcm_sw_params_current(mHandle, sw_params)<0){
    	LOGE("failed to fetch alsa current sw params");
	return BAD_VALUE;
    }

    snd_pcm_uframes_t bufferSize = 0;
    snd_pcm_uframes_t periodSize = 0;
    snd_pcm_uframes_t startThreshold;

   snd_pcm_get_params(mHandle, &bufferSize, &periodSize);

   startThreshold = (bufferSize / periodSize) * periodSize;
 
   if( snd_pcm_sw_params_set_start_threshold(mHandle, sw_params, startThreshold)<0 ){
	LOGW("Unable to set start threshold to %lu frames", startThreshold);
   }

   // Stop the transfer when the buffer is full.
   if( snd_pcm_sw_params_set_stop_threshold(mHandle, sw_params, bufferSize)<0 ){
	LOGW("Unable to set stop threshold to %lu frames", bufferSize);
   }
 
   // Allow the transfer to start when at least periodSize samples can be
   // processed.
   if( snd_pcm_sw_params_set_avail_min(mHandle, sw_params, periodSize)<0 ){
        LOGW("Unable to configure available minimum to %lu", periodSize);
   }
    // Commit the software parameters back to the device.
   if( snd_pcm_sw_params(mHandle, sw_params)<0 ){;
	LOGE("Unable to configure software parameters");
        return BAD_VALUE;
   }
    //snd_pcm_hw_params_free(params);

    if(format > 0)
        mFormat = format;
    if(channels > 0)
        mChannelCount = channels;
    if(rate > 0)
        mSampleRate = rate;
    mAudioHardware = hw;
    return NO_ERROR;
}

void AudioStreamOutMarvell::setOutputHandle(snd_pcm_t *handle)
{
    sem_wait(&mLock);
    mHandle = handle;
    sem_post(&mLock);
}

status_t AudioStreamOutMarvell::setVolume(float volume)
{
    if (mAudioHardware)
        mAudioHardware->setVolume(OUTPUT, volume);
    return NO_ERROR;
}

ssize_t AudioStreamOutMarvell::write(const void* buffer, size_t bytes)
{
    sem_wait(&mLock);
    if(!mHandle)
    {
        sem_post(&mLock);
        LOGE("---------------handle is null---------------\n");
        return (ssize_t)bytes;
    }

    int frames = bytes / sizeof(int16_t) / mChannelCount;
    int ret;
    int16_t *ptr = (int16_t*)buffer;

    while(frames > 0)
    {
        ret = snd_pcm_writei(mHandle, ptr, frames);    
        if(ret < 0) {   
            LOGE("---------------write error is %s---------------\n", snd_strerror(ret)); 
            if (ret == -EAGAIN) {
                continue;
            } else if (xrun_recovery (mHandle, ret) < 0) {
                sem_post(&mLock);
                return bytes;
                //return (ssize_t)((char*)ptr - (char*)buffer);
            }
            continue;
        }
        ptr += ret * mChannelCount;
        frames -= ret;
    }
    sem_post(&mLock);
    return (ssize_t)bytes;
}

status_t AudioStreamOutMarvell::dump(int fd, const Vector<String16>& args)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    snprintf(buffer, SIZE, "AudioStreamOutMarvell::dump\n");
    snprintf(buffer, SIZE, "\tsample rate: %d\n", sampleRate());
    snprintf(buffer, SIZE, "\tbuffer size: %d\n", bufferSize());
    snprintf(buffer, SIZE, "\tchannel count: %d\n", channelCount());
    snprintf(buffer, SIZE, "\tformat: %d\n", format());
    result.append(buffer);
    ::write(fd, result.string(), result.size());
    return NO_ERROR; 
}

// ----------------------------------------------------------------------------

AudioStreamInMarvell::AudioStreamInMarvell() 
{
    sem_init(&mLock, 0, 1);
    mFormat = AudioSystem::PCM_16_BIT;
    mSampleRate = 8000;
    mChannelCount = 1;
    mAudioHardware = NULL;
}

AudioStreamInMarvell::~AudioStreamInMarvell()
{
    if (mAudioHardware)
        mAudioHardware->closeInputStream(this);
    sem_destroy(&mLock);
}

status_t AudioStreamInMarvell::set(AudioHardwareMarvell *hw, int format, int channels, uint32_t rate, AudioSystem::audio_in_acoustics acoustics)
{
    // fix up defaults
    if (format == 0) format = mFormat;
    if (channels == 0) channels = channelCount();
    if (rate == 0) rate = sampleRate();
    // check values
    if (format != AudioSystem::PCM_16_BIT)
        return BAD_VALUE;

    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(mHandle, params);

    /* Set the desired hardware parameters. */
    snd_pcm_hw_params_set_access(mHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(mHandle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(mHandle, params, channels);
    snd_pcm_hw_params_set_rate_near(mHandle, params, &rate, NULL);

    /* Write the parameters to the driver */
    int ret = snd_pcm_hw_params(mHandle, params);
    if (ret < 0) {
        LOGE("************************unable to set hw parameters: %s***********************\n", snd_strerror(ret));
        return BAD_VALUE;
    }

    //snd_pcm_hw_params_free(params);

    if(format > 0)
        mFormat = format;
    if(channels > 0)
        mChannelCount = channels;
    if(rate > 0)
        mSampleRate = rate;
    mAudioHardware = hw;
    return NO_ERROR;
}

void AudioStreamInMarvell::setInputHandle(snd_pcm_t *handle)
{
    sem_wait(&mLock);
    mHandle = handle;
    sem_post(&mLock);
}

status_t AudioStreamInMarvell::setGain(float gain)
{
    if (mAudioHardware)
        mAudioHardware->setVolume(INPUT, gain);
    return NO_ERROR; 
}

ssize_t AudioStreamInMarvell::read(void* buffer, ssize_t bytes)
{
    sem_wait(&mLock);
    if(!mHandle)
    {
        sem_post(&mLock);
        LOGE("---------------handle is null---------------\n");
        return (ssize_t)bytes;
    }

    int frames = bytes / sizeof(int16_t) / mChannelCount;
    int ret;
    int16_t *ptr = (int16_t*)buffer;

    while(frames > 0)
    {
        ret = snd_pcm_readi(mHandle, ptr, frames);    
        if(ret < 0) {   
            LOGE("---------------read error is %s---------------\n", snd_strerror(ret)); 
            if (ret == -EAGAIN) {
                continue;
            } else if (xrun_recovery (mHandle, ret) < 0) {
                sem_post(&mLock);
                return bytes;
                //return (ssize_t)((char*)ptr - (char*)buffer);
            }
            continue;
        }
        ptr += ret * mChannelCount;
        frames -= ret;
    }
    sem_post(&mLock);
    return (ssize_t)bytes;
}

status_t AudioStreamInMarvell::dump(int fd, const Vector<String16>& args)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    snprintf(buffer, SIZE, "AudioStreamInMarvell::dump\n");
    result.append(buffer);
    snprintf(buffer, SIZE, "\tsample rate: %d\n", sampleRate());
    result.append(buffer);
    snprintf(buffer, SIZE, "\tbuffer size: %d\n", bufferSize());
    result.append(buffer);
    snprintf(buffer, SIZE, "\tchannel count: %d\n", channelCount());
    result.append(buffer);
    snprintf(buffer, SIZE, "\tformat: %d\n", format());
    result.append(buffer);
    ::write(fd, result.string(), result.size());
    return NO_ERROR;
}

// ----------------------------------------------------------------------------

AudioHardwareInterface* createAudioHardware(void)
{
    return new AudioHardwareMarvell();
}

}; // namespace android
