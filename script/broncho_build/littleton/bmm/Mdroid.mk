LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_CFLAGS := -mabi=aapcs-linux
LOCAL_SRC_FILES := bmm_lib.c
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := libutils libcutils libdl
LOCAL_MODULE := libMrvlBMM
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
bmm_module := $(TARGET_OUT)/lib/modules/bmm.ko
$(bmm_module) : $(LOCAL_PATH)/bmm.ko | $(ACP)
	$(transform-prebuilt-to-target)

ALL_PREBUILT += $(bmm_module)

