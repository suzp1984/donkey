
#ifndef ANDROID_AUDIO_HARDWARE_Marvell_H
#define ANDROID_AUDIO_HARDWARE_Marvell_H

#include <stdint.h>
#include <sys/types.h>
#include <semaphore.h>
#include <alsa/asoundlib.h>

#include <hardware_legacy/AudioHardwareBase.h>


namespace android {

class AudioHardwareMarvell;

// ----------------------------------------------------------------------------

class AudioStreamOutMarvell : public AudioStreamOut {
public:
                        AudioStreamOutMarvell();
    virtual             ~AudioStreamOutMarvell();

    status_t            set(AudioHardwareMarvell *hw, int format, int channels, uint32_t rate);
    void                setOutputHandle(snd_pcm_t *handle);

    virtual uint32_t    sampleRate() const { return mSampleRate; }
    virtual size_t      bufferSize() const { return 4096; }
    virtual int         channelCount() const { return mChannelCount; }
    virtual int         format() const { return mFormat; }
    virtual status_t    setVolume(float volume);
    virtual ssize_t     write(const void* buffer, size_t bytes);
    virtual status_t    dump(int fd, const Vector<String16>& args);
    virtual uint32_t 	latency() const {return 0;}
    virtual status_t	standby() {return NO_ERROR;}

private:
    AudioHardwareMarvell    *mAudioHardware;
    int                    mFormat;
    int                    mChannelCount;
    uint32_t            mSampleRate;
    sem_t                mLock;
    snd_pcm_t            *mHandle;
    unsigned int	  buffer_time;
    unsigned int	  period_time;
};

class AudioStreamInMarvell : public AudioStreamIn {
public:
                        AudioStreamInMarvell();
    virtual             ~AudioStreamInMarvell();

    status_t            set(AudioHardwareMarvell *hw, int format, int channels, uint32_t rate, AudioSystem::audio_in_acoustics acoustics);
    void                setInputHandle(snd_pcm_t *handle);

    virtual uint32_t    sampleRate() const { return mSampleRate; }
    virtual size_t      bufferSize() const { return 320; }
    virtual int         channelCount() const { return mChannelCount; }
    virtual int         format() const { return mFormat; }
    virtual status_t    setGain(float gain);
    virtual ssize_t     read(void* buffer, ssize_t bytes);
    virtual status_t    dump(int fd, const Vector<String16>& args);
    virtual status_t	standby() { return NO_ERROR;}

private:
    AudioHardwareMarvell *mAudioHardware;
    int                    mFormat;
    int                    mChannelCount;
    uint32_t            mSampleRate;
    sem_t                mLock;
    snd_pcm_t            *mHandle;
};

class AudioHardwareMarvell : public  AudioHardwareBase
{
public:
                        AudioHardwareMarvell();
    virtual             ~AudioHardwareMarvell();
    virtual status_t    initCheck();
    virtual status_t    standby();
    virtual status_t    setVoiceVolume(float volume);
    virtual status_t    setMasterVolume(float volume);

    // routing
    virtual status_t    setRouting(int mode, uint32_t routes);
    virtual status_t    getRouting(int mode, uint32_t* routes);
    
    // mode
    virtual status_t    setMode(int mode);
    virtual status_t    getMode(int* mode);
	
    // mic mute
    virtual status_t    setMicMute(bool state);
    virtual status_t    getMicMute(bool* state);

    virtual status_t    setParameter(const char* key, const char* value);

    // create I/O streams
    virtual AudioStreamOut* openOutputStream(
                                int format=0,
                                int channelCount=0,
                                uint32_t sampleRate=0,
                                status_t *status=0);

    virtual AudioStreamIn* openInputStream(
                                int inputSource,
                                int format,
                                int channelCount,
                                uint32_t sampleRate,
                                status_t *status,
				AudioSystem::audio_in_acoustics acoustics);

    void                closeOutputStream(AudioStreamOutMarvell* out);
    void                closeInputStream(AudioStreamInMarvell* in);
    void                setVolume(int direction, float volume);
    status_t           enableAlsaDevice(const char* name);
    status_t           disableAlsaDevice();
protected:
    virtual status_t doRouting();	
    virtual status_t    dump(int fd, const Vector<String16>& args);

    bool                mMicMute;
private:
    status_t            dumpInternals(int fd, const Vector<String16>& args);

    AudioStreamOutMarvell    *mOutput;
    AudioStreamInMarvell    *mInput;
    int                    mInputFormat;
    int                    mInputChannelCount;
    uint32_t            mInputSampleRate;
    int                    mOutputFormat;
    int                    mOutputChannelCount;
    uint32_t            mOutputSampleRate;	
    sem_t               mLock;
    snd_pcm_t        *mInputHandle;
    snd_pcm_t        *mOutputHandle;
    float                  mVoiceVolume;	

    int             mOldMode;
    uint32_t        mOldRouting;	
};

// ----------------------------------------------------------------------------

AudioHardwareInterface * createAudioHardware(void);

}; // namespace android

#endif // ANDROID_AUDIO_HARDWARE_Marvell_H
