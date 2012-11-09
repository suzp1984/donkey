#include "charger_test.h"
#include "ui.h"
#include "minui.h"
#include "engapi.h"
#include "engat.h"

#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#define LOG_TAG "engtest"
#include <utils/Log.h>

#define SPRD_CALI_MASK          0x00000200
#define ENG_BATVOL      "/sys/class/power_supply/battery/real_time_voltage"
#define ENG_CHRVOL      "/sys/class/power_supply/battery/charger_voltage"
#define ENG_CURRENT     "/sys/class/power_supply/battery/real_time_current"
#define ENG_USBONLINE   "/sys/class/power_supply/usb/online"
#define ENG_ACONLINE    "/sys/class/power_supply/ac/online"

typedef struct {
	int thread_run;
} PrivInfo;

unsigned int eng_chrtest_adccali(void)
{
	int fd;
	unsigned int ret=0;
	char cmd[64];
	char cali[16];
	char *ptr, *start_ptr, *end_ptr;
	fd = engapi_open(0);

	if (fd < 0)
		return 0;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%d,%d,%s",ENG_AT_NOHANDLE_CMD,1,"AT+SGMR=0,0,4");
	engapi_write(fd, cmd, strlen(cmd));
	memset(cmd, 0, sizeof(cmd));
	engapi_read(fd, cmd, sizeof(cmd));

	if(strstr(cmd,"ERR") != NULL)
		return 0;

	ptr = strchr(cmd, ':');
	ptr++;
	while(isspace(*ptr)||(*ptr==0x0d)||(*ptr==0x0a))
		ptr++;

	start_ptr = ptr;

	while(!isspace(*ptr)&&(*ptr!=0x0d)&&(*ptr!=0x0a))
		ptr++;

	end_ptr = ptr;

	memset(cali, 0, sizeof(cali));
	snprintf(cali, end_ptr-start_ptr+1, start_ptr);
	ret = strtoul(cali, 0, 16);

	engapi_close(fd);

	if((ret&SPRD_CALI_MASK)>0) {
		ret = 1;
	} else {
		ret = 0;
	}

	return ret;
}

float eng_chrtest_batvol(void)
{
	int fd=-1;
	int voltage=0, n=0;
	float vol=0.000;
	char buffer[16];

	fd = open(ENG_BATVOL, O_RDONLY);
	if(fd > 0){
		memset(buffer, 0, sizeof(buffer));
		n = read(fd, buffer, sizeof(buffer));
		if(n > 0) {
			voltage = atoi(buffer);
			vol = ((float) voltage) * 0.001;
		}
		close(fd);
	}

	return vol;
}

float eng_chrtest_chrvol(void)
{
	int fd=-1;
	int voltage=0, n=0;
	float vol=0.000;
	char buffer[16];

	fd = open(ENG_CHRVOL, O_RDONLY);
	if(fd > 0){
		memset(buffer, 0, sizeof(buffer));
		n = read(fd, buffer, sizeof(buffer));
		if(n > 0) {
			voltage = atoi(buffer);
			vol = ((float) voltage) * 0.001;
		}
		close(fd);
	}

	return vol;
}

void eng_chrtest_chrcur(char *current, int length)
{
	int fd=-1;
	int n=0;

	fd = open(ENG_CURRENT, O_RDONLY);
	if(fd > 0){
		n = read(fd, current, length);
		if(n > 0) {
			LOGE("%s: current=%s;\n", __func__, current);
		}
		close(fd);
	}
}

int eng_chrtest_usbin(void)
{
	int fd=-1;
	int usbin=0, n=0;
	char buffer[16];

	fd = open(ENG_USBONLINE, O_RDONLY);
	if(fd > 0){
		memset(buffer, 0, sizeof(buffer));
		n = read(fd, buffer, sizeof(buffer));
		if(n > 0) {
			usbin = atoi(buffer);
			LOGE("%s: buffer=%s; usbin=%d;\n", __func__, buffer, usbin);
		}
		close(fd);
	}

	return usbin;
}

int eng_chrtest_acin(void)
{
	int fd=-1;
	int acin=0, n=0;
	char buffer[16];

	fd = open(ENG_ACONLINE, O_RDONLY);
	if(fd > 0){
		memset(buffer, 0, sizeof(buffer));
		n = read(fd, buffer, sizeof(buffer));
		if(n > 0) {
			acin = atoi(buffer);
			LOGE("%s: buffer=%s; usbin=%d;\n", __func__, buffer, acin);
		}
		close(fd);
	}

	return acin;
}

static void* charger_test_thread(void* ctx)
{
	TestCase* thiz = (TestCase*)ctx;
	DECLES_PRIV(priv, thiz);

	float vol=0.0, chrvol=0.0;
	int current=0, usbin=0, acin=0, chrin=0;
	unsigned int cali;
	char buffer[64]; 
	char tmpbuf[32];
	int width = gr_fb_width()/2-130;
	int height = gr_fb_height()/2;
	int draw_y = 100;


	while(priv->thread_run == 1) {
		draw_y = 100;
		ui_screen_clean();
		memset(tmpbuf, 0, sizeof(tmpbuf));
		cali = eng_chrtest_adccali();
		vol = eng_chrtest_batvol();
		chrvol = eng_chrtest_chrvol();
		usbin = eng_chrtest_usbin();
		acin = eng_chrtest_acin();
		eng_chrtest_chrcur(tmpbuf, sizeof(tmpbuf));

		if (usbin == 1 || acin == 1)
			chrin = 1;
		else 
			chrin = 0;

		gr_color(255, 255, 255, 255);
		ui_draw_title(UI_TITLE_START_Y, CHARGER_TEST_CASE);
		memset(buffer, 0, sizeof(buffer));
		if (cali == 1) {
			gr_color(0, 255, 0, 255);
			sprintf(buffer, "Battery Calibration: Yes");
		} else {
			gr_color(255, 0, 0, 255);
			sprintf(buffer, "Battery Calibration: No");
		}

		ui_draw_title(draw_y, buffer);
		draw_y += CHAR_HEIGHT;

		//show battery voltage
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "%s%.2f %s", "Battery Voltage: ", vol, "V");
		gr_color(0, 255, 255, 255);
		ui_draw_title(draw_y, buffer);
		draw_y += CHAR_HEIGHT;

		//show charing
		memset(buffer, 0, sizeof(buffer));
		if(chrin==1) {
			gr_color(0, 255, 0, 255);
			sprintf(buffer, "%s", "Charging: YES");
		} else {
			gr_color(255, 0, 0, 255);
			sprintf(buffer, "%s", "Charging: NO");
		}
		ui_draw_title(draw_y, buffer);
		draw_y += CHAR_HEIGHT;

		//show charger type
		memset(buffer, 0, sizeof(buffer));
		if(usbin==1) {
			gr_color(255, 255, 0, 255);
			sprintf(buffer, "%s", "Charger Type: USB");
		} else if (acin==1){
			gr_color(0, 255, 0, 255);
			sprintf(buffer, "%s", "Charger Type: AC");
		} else {
			gr_color(0, 0, 255, 255);
			sprintf(buffer, "%s", "Charger Type: NO");
		}
		ui_draw_title(draw_y, buffer);
		draw_y += CHAR_HEIGHT;

		//show charger voltage
		sprintf(buffer, "%s%.2f %s", "Charger Voltage: ", chrvol, "V");
		gr_color(0, 255, 0, 255);
		ui_draw_title(draw_y, buffer);
		draw_y += CHAR_HEIGHT;

		//show charger current
		memset(buffer, 0, sizeof(buffer));
		
		if (isdigit(tmpbuf[0]))
			sprintf(buffer, "%s%s%s", "Charging Current: ", tmpbuf, "mA");
		else
			sprintf(buffer, "%s%s", "Charging Current: ", tmpbuf);
		ui_draw_title(draw_y, buffer);

		ui_draw_softkey();
		gr_flip();

		sleep(1);
	}

	return NULL;
}

static int charger_test_run(TestCase* thiz)
{
	DECLES_PRIV(priv, thiz);
	pthread_t t;

	ui_screen_clean();
	priv->thread_run = 1;
	pthread_create(&t, NULL, charger_test_thread, (void*)thiz);

	thiz->passed = ui_draw_handle_softkey(thiz->name);

	priv->thread_run = 0;
	pthread_join(t, NULL);
	return 0;
}

static void charger_test_destroy(TestCase* thiz)
{
	SAFE_FREE(thiz);
}

TestCase* charger_test_create(int passed)
{
	TestCase* thiz = (TestCase*)malloc(sizeof(TestCase) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);
		thiz->run = charger_test_run;
		thiz->destroy = charger_test_destroy;

		thiz->name = CHARGER_TEST_CASE;
		thiz->passed = passed;
		priv->thread_run = 0;
	}

	return thiz;
}
