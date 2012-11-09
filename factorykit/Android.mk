# install prebuilt lib
LOCAL_PATH := $(call my-dir)

# buid lib
include $(CLEAR_VARS)

commands_local_path := $(LOCAL_PATH)

## -D$(BOARD_PRODUCT_NAME)
LOCAL_CFLAGS += -D_POSIX_SOURCE 

LOCAL_C_INCLUDES    +=  $(LOCAL_PATH) \
			hardware/sprd/hsdroid/libcamera \
			$(LOCAL_PATH)/ui \
			$(LOCAL_PATH)/ui/minui \
			external/sqlite/dist/ \
			$(LOCAL_PATH)/common \
			external/sprd/engineeringmodel/engcs \
			external/sprd/alsa/alsa-lib/include/ \
			system/bluetooth/bluez-clean-headers \
			external/expat/lib

LOCAL_STATIC_LIBRARIES += libfkminui libpixelflinger_static libcutils

LOCAL_SHARED_LIBRARIES := libsqlite libasound  libengclient libeng_audio_mode libexpat

LOCAL_SRC_FILES := main.c  \
				   ui/ui.c \
				   menu.c \
				   fk_globals.c \
				   common.c \
				   fk_sqlite.c \
				   test_case_manager.c \
				   test_case_factory.c \
				   fk_config_expat_xml.c \
				   hello_test_case.c \
				   version_test_case.c \
				   rfcali_test_case.c \
				   lcd_test_case.c \
				   vibrator_test_case.c \
				   backlight_test_case.c \
				   phone_loopback_test_case.c \
				   camera_test_case.c \
				   speaker_test_case.c \
				   sd_test.c \
				   rtc_test.c \
				   phone_call_test.c \
				   charger_test.c \
				   headset_test.c \
				   sim_test.c \
				   fm_test.c \
				   tp_test.c \
				   multi_touch_test.c \
				   key_test.c \
				   gsensor_test.c \
				   wifi_test.c \
				   bluetooth_test.c \
				   psensor_test.c \
				   lsensor_test.c \
				   twinkle_light_test.c \
				   gps_test.c


LOCAL_MODULE := factorykit 

include $(BUILD_EXECUTABLE)

include $(commands_local_path)/ui/minui/Android.mk

