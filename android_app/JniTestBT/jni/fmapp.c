/*********************************************************************
 *
 *  Copyright (c) 2010, Broadcom Corp., All Rights Reserved.
 *  Broadcom Bluetooth Core. Proprietary and confidential.
 *
 *********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "fmradio.h"

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
}

void
fm_get_channel_freq_cb(uint16_t param)
{
	fm_block.freq = param;
	printf("in cb, current freq =%d, %dKHz\n", param, param + 64000);
}

void
fm_get_current_rssi_cb(uint8_t param)
{
	fm_block.rssi = ~param;
	printf("in cb, current rssi = -%d dBm\n", (uint8_t)~param);
}

void
fm_get_search_steps_cb(uint8_t param)
{
	fm_block.steps = param;
	printf("in cb, search steps = %dkHz\n", param);
}

void
fm_get_mute_state_cb(uint8_t param)
{
	fm_block.mute_state = param;
	if (param == FM_MUTE_OFF)
		printf("in cb, mute_state = mute off\n");
	else
		printf("in cb, mute_state = mute on\n");
}

void
fm_get_volume_gain_cb(uint16_t param)
{
	fm_block.volume = param;
	printf("in cb, volume gain = %d\n", param);
}

void
fm_get_preset_channels_cb(uint8_t plen, uint8_t *param)
{
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
}

void
fm_get_search_rssi_threshold_cb(uint8_t param)
{
	printf("in cb, rssi_threshold = -%ddBm\n", param);
}

void 
fm_get_stereo_mono_status_cb(uint8_t param)
{
	if (param == FM_AUDIO_STEREO)
		printf("in cb, audio STEREO\n");
	else
		printf("in cb, audio MONO\n");
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
cmd_fmon(char *cmdparam)
{
	fm_func_on(&fm_block);
}
void
cmd_fmoff(char *cmdparam)
{
	fm_func_off();
}
void
cmd_tunefreq(char *cmdparam)
{
	uint32_t freq;
	
	sscanf(cmdparam, "%d", &freq);
	freq -= 64000;
	fm_tune_freq((uint16_t)freq);
}
void
cmd_getfreq(char *cmdparam)
{
	fm_get_current_freq();
}

void
cmd_setthresh(char *cmdparam)
{
	uint16_t thresh;
	
	sscanf(cmdparam, "%hu", &thresh);
	fm_block.rssi_thresh = thresh;
	fm_set_search_rssi_threshold((uint8_t)thresh);
	fm_get_search_rssi_threshold();
}

void
cmd_getthresh(char *cmdparam)
{
	fm_get_search_rssi_threshold();
}

void
cmd_getrssi(char *cmdparam)
{
	fm_get_current_rssi();
}

void
cmd_searchup(char *cmdparam)
{
	uint32_t freq;
	
	sscanf(cmdparam, "%d", &freq);
	freq -= 64000;
	if (freq > 44000)
		freq = fm_block.freq +100;

	fm_search(FM_SEARCH_UP, (uint16_t)freq);
}

void
cmd_searchdown(char *cmdparam)
{
	uint32_t freq;
	
	sscanf(cmdparam, "%d", &freq);
	freq -= 64000;
	if (freq > 44000)
		freq = fm_block.freq -100;
	fm_search(FM_SEARCH_DOWN, (uint16_t)freq);
}

void
cmd_autoup(char *cmdparam)
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
cmd_autodown(char *cmdparam)
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
cmd_setsteps50(char *cmdparam)
{
	fm_set_search_steps(FM_SEARCH_STEPS_50KHz);
	fm_get_search_steps();
}

void
cmd_setsteps100(char *cmdparam)
{
	fm_set_search_steps(FM_SEARCH_STEPS_100KHz);
	fm_get_search_steps();
}

void
cmd_getsteps(char *cmdparam)
{
	fm_get_search_steps();
}

void
cmd_mono(char *cmdparam)
{
	fm_set_region_tune_mode(fm_block.region_tune_mode & 0x1);
	fm_get_region_tune_mode();
}

void
cmd_stereo(char *param)
{
	fm_set_region_tune_mode((fm_block.region_tune_mode & 0x1)|0x2);
	fm_get_region_tune_mode();
}

void
cmd_setvol(char *cmdparam)
{
	uint16_t vol;
	
	sscanf(cmdparam, "%hu", &vol);
	fm_set_volume(vol);
}

void
cmd_getvol(char *cmdparam)
{
	fm_get_volume();
}

void
cmd_mute(char *cmdparam)
{
	fm_mute();
}

void
cmd_unmute(char *cmdparam)
{
	fm_unmute();
}

void
cmd_getmute(char *cmdparam)
{
	fm_get_mute_state();
}

void
cmd_abort(char *cmdparam)
{
	fm_searchabort();
}

void
cmd_getaudiostatus(char *cmdparam)
{
	fm_get_stereo_mono_status();
}

static struct {
	const char *cmd;
	void (*func)(char *cmdparam);
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

int main(int argc, char *argv[])
{
	char s[80];
	int i;
	int r;
	// need start the bluetoothd daemon and
	// start the service hciattach(brcm_patchram_plus)

	r = fm_init(&fm_cbs);
	if (r != 0) {
		printf("fm_init failed.\n");
		return -1;
	}
		

	r = fm_func_on(&fm_block);
	if (r != 0) {
		printf("fm_func_on failed.\n");
		return -2;
	}


		
	for (;;) {
		// printf usage
		printf("fmradio function demo\n");
		printf("Parameter:\n");
		printf("\t[freq]\t\tthe frequence number, in unit 1KHz. Optional.\n");
		printf("\t<rssi>\t\tthe rssi threshold number , in dBm.\n");
		printf("\t<channels>\tthe channels number for auto search.\n");
		printf("\t<volume>\tthe volume gain number, 1 for MIN, 256 for MAX.\n");
		printf("Commands:\n");
		for (i=0;command[i].cmd;i++) {
			printf("\t%-6s\t - %s\n", command[i].cmd,command[i].doc);
		}
		printf(">>");
		memset(s, 0, 80);
		fgets(s, 80, stdin);
		
		for (i=0; command[i].cmd;i++) {
			if (strncmp(command[i].cmd, s, strlen(command[i].cmd))) {
				if (command[i+1].cmd == NULL)
					printf("unknown cmd\n");
				continue;
			}
			if (strncmp(command[i].cmd, "exit", strlen("exit"))) {
				command[i].func(s+strlen(command[i].cmd));
				break;
			}
			else {
				//exit
				fm_func_off();
				fm_close();
				return 0;
			}
	
		}
	}
}
