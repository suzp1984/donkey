/*********************************************************************
 *
 *  Copyright (c) 2010, Broadcom Corp., All Rights Reserved.
 *  Broadcom Bluetooth Core. Proprietary and confidential.
 *
 *********************************************************************/

#ifndef _FMRADIO_H
#define _FMRADIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* for the byte of region_tune_mode */
/* bit0, 0=Europe, 1=Japan */
#define FM_REGION_EUR	0x00
#define FM_REGION_JAN	0x01
/* bit1, 0=mono, 1=auto stereo */
#define FM_TUNE_MONO		0x00
#define FM_TUNE_AUTOSTEREO	0x02

#define FM_AUDIO_MONO   0x01
#define FM_AUDIO_STEREO 0X02

#define FM_SEARCH_STEPS_100KHz	100
#define FM_SEARCH_STEPS_50KHz	50

#define FM_MUTE_OFF	0X00
#define FM_MUTE_ON	0x01

#define FM_SEARCH_DOWN 0x00
#define FM_SEARCH_UP 	0x01


typedef struct {
	uint16_t	freq;//Initialize frequence, setting in 1 kHz offset from 64MHz. For example, 23500 means 87.5 MHz
	uint16_t	volume;//FM volume gain, valid data from 1 (MIN) to 256(MAX).
	uint8_t		steps;//Must be 100 (kHz) or 50 (kHz)
	uint8_t		region_tune_mode;//Bit0: 0 = Europe, 1 = Japan;Bit1: 0 = mono, 1 = stereo
	uint8_t		rssi;//Current rssi value
	uint8_t		rssi_thresh;//Search rssi threshold(jixian)
	uint8_t		mute_state;//0 = mute off, 1 = mute on
}fm_ctrl_block;

typedef void (*fm_get_region_tune_mode_callback)(uint8_t param);
typedef void (*fm_get_channel_freq_callback)(uint16_t param);
typedef void (*fm_get_current_rssi_callback)(uint8_t param);
typedef void (*fm_get_search_steps_callback)(uint8_t param);
typedef void (*fm_get_mute_state_callback)(uint8_t param);
typedef void (*fm_get_volume_gain_callback)(uint16_t param);
typedef void (*fm_get_preset_channels_callback)(uint8_t plen, uint8_t *param);
typedef void (*fm_get_search_rssi_threshold_callback)(uint8_t param);
typedef void (*fm_get_stereo_mono_status_callback)(uint8_t param);

typedef struct {
	fm_get_region_tune_mode_callback	region_tune_cb;
	fm_get_channel_freq_callback	channel_freq_cb;
	fm_get_current_rssi_callback	current_rssi_cb;
	fm_get_search_steps_callback	search_steps_cb;
	fm_get_mute_state_callback	mute_state_cb;
	fm_get_volume_gain_callback	volume_gain_cb;
	fm_get_preset_channels_callback	preset_channel_cb;
	fm_get_search_rssi_threshold_callback	rssi_threshold_cb;
	fm_get_stereo_mono_status_callback	stereo_mono_status_cb;
}fm_callbacks;



int fm_init(fm_callbacks *cbs);
int fm_func_on(fm_ctrl_block *block);
int fm_get_region_tune_mode(void);
int fm_set_region_tune_mode(uint8_t mode);
int fm_tune_freq(uint16_t freq);
int fm_get_current_freq(void);
int fm_set_search_rssi_threshold(uint8_t threshold);
int fm_get_search_rssi_threshold(void);
int fm_get_current_rssi(void);
int fm_search(uint8_t direct, uint16_t freq);
int fm_auto_search(uint8_t direct, uint16_t freq, uint8_t num);
int fm_searchabort(void);
int fm_set_search_steps(uint8_t steps);
int fm_get_search_steps(void);
int fm_mute(void);
int fm_unmute(void);
int fm_get_mute_state(void);
int fm_set_volume(uint16_t volume);
int fm_get_volume(void);
int fm_func_off(void);
int fm_close(void);
int fm_get_stereo_mono_status(void);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* _FMRADIO_H */
