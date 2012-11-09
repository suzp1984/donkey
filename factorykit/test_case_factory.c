#include "test_case_factory.h"
#include <string.h>
#include "fk_sqlite.h"

#include "hello_test_case.h"
#include "version_test_case.h"
#include "rfcali_test_case.h"
#include "lcd_test_case.h"
#include "vibrator_test_case.h"
#include "backlight_test_case.h"
#include "phone_loopback_test_case.h"
#include "camera_test_case.h"
#include "speaker_test_case.h"
#include "sd_test.h"
#include "rtc_test.h"
#include "phone_call_test.h"
#include "charger_test.h"
#include "headset_test.h"
#include "sim_test.h"
#include "fm_test.h"
#include "tp_test.h"
#include "multi_touch_test.h"
#include "key_test.h"
#include "gsensor_test.h"
#include "wifi_test.h"
#include "bluetooth_test.h"
#include "psensor_test.h"
#include "lsensor_test.h"
#include "twinkle_light_test.h"
#include "gps_test.h"

struct _TestCaseFactory {
	char priv[1];
};

static TestCaseFactory* test_case_factory_create()
{
	TestCaseFactory* thiz = (TestCaseFactory*) malloc(sizeof(TestCaseFactory));

	return thiz;
}

TestCase* test_case_factory_get(const char* name)
{
	int passed = -1;
	if (factory == NULL) {
		factory = test_case_factory_create();
	}

	passed = fk_sql_str2int_get(name);

	if (!strncmp(name, HELLO_CASE_NAME, strlen(HELLO_CASE_NAME))) {
		return hello_test_case_create(passed);
	} else if (!strncmp(name, VERSION_TEST_CASE, strlen(VERSION_TEST_CASE))) {
		return version_test_case_create(passed);
	} else if (!strncmp(name, RFCALI_TEST_CASE, strlen(RFCALI_TEST_CASE))) {
		return rfcali_test_case_create(passed);
	} else if (!strncmp(name, LCD_TEST_CASE, strlen(LCD_TEST_CASE))) {
		return lcd_test_case_create(passed);
	} else if (!strncmp(name, VIBRATOR_TEST_CASE, strlen(VIBRATOR_TEST_CASE))) {
		return vibrator_test_case_create(passed);
	} else if (!strncmp(name, BACKLIGHT_TEST_CASE, strlen(BACKLIGHT_TEST_CASE))) {
		return backlight_test_case_create(passed);
	} else if (!strncmp(name, PHONE_LOOPBACK_TEST_CASE, strlen(PHONE_LOOPBACK_TEST_CASE))) {
		return phone_loopback_test_create(passed);
	} else if (!strncmp(name, CAMERA_TEST_CASE, strlen(CAMERA_TEST_CASE))) {
		return camera_test_case_create(passed);
	} else if (!strncmp(name, SPEAKER_TEST_CASE, strlen(SPEAKER_TEST_CASE))) {
		return speaker_test_case_create(passed);
	} else if (!strncmp(name, SD_TEST_CASE, strlen(SD_TEST_CASE))) {
		return sd_test_create(passed);
	} else if (!strncmp(name, RTC_TEST_CASE, strlen(RTC_TEST_CASE))) {
		return rtc_test_create(passed);
	} else if (!strncmp(name, PHONE_CALL_TEST, strlen(PHONE_CALL_TEST))) {
		return phone_call_test_create(passed);
	} else if (!strncmp(name, CHARGER_TEST_CASE, strlen(CHARGER_TEST_CASE))) {
		return charger_test_create(passed);
	} else if (!strncmp(name, HEADSET_TEST_CASE, strlen(HEADSET_TEST_CASE))) {
		return headset_test_create(passed);
	} else if (!strncmp(name, SIM_TEST_CASE, strlen(SIM_TEST_CASE))) {
		return sim_test_create(passed);
	} else if (!strncmp(name, FM_TEST_CASE, strlen(FM_TEST_CASE))) {
		return fm_test_create(passed);
	} else if (!strncmp(name, TP_TEST_CASE, strlen(TP_TEST_CASE))) {
		return tp_test_create(passed);
	} else if (!strncmp(name, MULTI_TOUCH_TEST_CASE, strlen(MULTI_TOUCH_TEST_CASE))) {
		return multi_touch_test_create(passed);
	} else if (!strncmp(name, KEY_TEST_CASE, strlen(KEY_TEST_CASE))) {
		return key_test_create(passed);
	} else if (!strncmp(name, GSENSOR_TEST_CASE, strlen(GSENSOR_TEST_CASE))) {
		return gsensor_test_create(passed);
	} else if (!strncmp(name, WIFI_TEST_CASE, strlen(WIFI_TEST_CASE))) {
		return wifi_test_create(passed);
	} else if (!strncmp(name, BLUETOOTH_TEST_CASE, strlen(BLUETOOTH_TEST_CASE))) {
		return bluetooth_test_create(passed);
	} else if (!strncmp(name, PSENSOR_TEST_CASE, strlen(PSENSOR_TEST_CASE))) {
		return psensor_test_create(passed);
	} else if (!strncmp(name, LSENSOR_TEST_CASE, strlen(LSENSOR_TEST_CASE))) {
		return lsensor_test_create(passed);
	} else if (!strncmp(name, TWINKLE_LIGHT_TEST_CASE, strlen(TWINKLE_LIGHT_TEST_CASE))) {
		return twinkle_light_test_create(passed);
	} else if (!strncmp(name, GPS_TEST_CASE, strlen(GPS_TEST_CASE))) {
		return gps_test_create(passed);
	} else {
		return NULL;
	}
}

#ifdef TEST_CASE_FACTORY_TEST

int main(int argc, char* argv[])
{
	int passed = -1;
	char* case_name;
	TestCase* hello = test_case_factory_get("hello");
	passed = test_case_get_status(hello);
	test_case_get_name(hello, (const char**)&case_name);
	printf("name: %s, passed: %d\n", case_name, passed);

	test_case_run(hello);
	passed = test_case_get_status(hello);
	printf("name: %s, passed: %d\n", case_name, passed);

	test_case_destroy(hello);

	return 0;
}

#endif // TEST_CASE_FACTORY_TEST
