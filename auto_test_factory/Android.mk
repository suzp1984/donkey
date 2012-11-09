LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_CFLAGS += -D_POSIX_SOURCE -lm -lpthread

LOCAL_C_INCLUDES += $(LOCAL_PATH) \
					external/sprd/alsa/alsa-lib/include/ \
					external/sprd/engineeringmodel/engcs \
					system/bluetooth/bluez-clean-headers

LOCAL_SRC_FILES := parcel.c \
				   cmd_listener.c \
				   main.c  \
				   dlist.c \
				   locker_nest.c \
				   locker_pthread.c \
				   queue.c \
				   version_info_cmd.c \
				   backlight_cmd.c \
				   getoption_cmd.c \
				   utils.c \
				   tp_cmd.c \
				   vibrator_cmd.c \
				   echoloop_cmd.c \
				   echoloop2_cmd.c \
				   keypress_cmd.c \
				   receive_cmd.c \
				   lcd_cmd.c \
				   ring_cmd.c \
				   headset_cmd.c \
				   cft_cmd.c \
				   flashlight_cmd.c \
				   led_cmd.c \
				   pl_sensor_cmd.c \
				   charge_cmd.c \
				   fm_cmd.c \
				   memorycard_cmd.c \
				   rtc_cmd.c \
				   camera_cmd.c \
				   camera1_cmd.c \
				   gsensor_cmd.c \
				   sim_cmd.c \
				   bluetooth_cmd.c \
				   wifi_cmd.c \
				   over_cmd.c \
				   phonecall_cmd.c \
				   keybl_cmd.c \
				   msensor_cmd.c

LOCAL_STATIC_LIBRARIES := libcutils

LOCAL_SHARED_LIBRARIES := libasound libengclient 

LOCAL_MODULE:= auto_test
#LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
