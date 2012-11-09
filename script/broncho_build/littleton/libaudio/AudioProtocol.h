#ifndef __AUDIO_PROTOCOL_H__
#define __AUDIO_PROTOCOL_H__

#include <stdint.h>

typedef struct {
	int client_type;
	int method_type;
} AUDIOIPCHEADER;

#define INVALIDIPCHEADER {-1, -1}

// Different types of client plugin for the daemon
#define INVALID_CLIENT_TYPE     0xFFFFFFFF
#define HIFI_CLIENT		0x00000001
#define VOICE_CLIENT		0x00000002
#define PHONE_CLIENT		0x00000003

#define PLUGIN_CTL_WRITE  0x00000001
#define PLUGIN_CTL_READ   0x00000002
#define PLUGIN_PCM_WRITE  0x00000003
#define PLUGIN_PCM_READ   0x00000004

// Different methods
#define INVALID_METHOD_ID     0xFFFFFFFF
#define METHOD_ENABLE 0x00000001
#define METHOD_DISABLE 0x00000002
#define METHOD_MUTE 0x00000003
#define METHOD_VOLUME_SET 0x00000004

#define INVALID_DEVICE     0xFFFFFFFF
#define EARPIECE_DEVICE 0x00000001
#define SPEAKER_DEVICE 0x00000002
#define BLUETOOTH_DEVICE 0x00000003
#define HEADSET_DEVICE 0x00000004

#define INPUT 0x00000001
#define OUTPUT 0x00000002

typedef struct {
	int device_type;
	int method_id;
	int param;
} __attribute__ ((packed)) AUDIOIPCDATA;

#define INVALIDIPCDATA {-1, -1, -1}

typedef struct {
	int  volume;
	int status;
} __attribute__ ((packed)) AUDIOCONTROLINFOS;


#define INVALIDCONTROLINFOS   { 0, 0, 0 }


typedef struct {
	unsigned int  format;
	unsigned short rate;
	unsigned char channels;
} __attribute__ ((packed)) AUDIOSTREAMINFOS;


#define INVALIDAUDIOSTREAMINFOS   { 0, 0, 0 }


/* Phone Status */
#define PHONE_STATUS_NOACTION			  0x00000000
#define PHONE_VOICECALL_DISABLE			  0x00000001
#define PHONE_VOICECALL_ENABLE			  0x00000002
#define PHONE_PCMSTREAM_PLAYBACK_DISABLE	  0x00000003
#define PHONE_PCMSTREAM_PLAYBACK_ENABLE		  0x00000004
#define PHONE_PCMSTREAM_CAPTURE_DISABLE		  0x00000005
#define PHONE_PCMSTREAM_CAPTURE_ENABLE		  0x00000006

/* Voice Status */
#define VOICE_STATUS_NOACTION	  0x00000000
#define VOICE_PLAYBACK_DISABLE	  0x00000001
#define VOICE_PLAYBACK_ENABLE	  0x00000002
#define VOICE_CAPTURE_DISABLE	  0x00000003
#define VOICE_CAPTURE_ENABLE	  0x00000004

#endif
