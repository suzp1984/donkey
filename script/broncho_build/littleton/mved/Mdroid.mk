LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

$(call add-prebuilt-files, STATIC_LIBRARIES, libmved.a)

include $(CLEAR_VARS)
LOCAL_WHOLE_STATIC_LIBRARIES := libmved

LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := libMrvlBMM
LOCAL_MODULE := libMrvlMVED
include $(BUILD_SHARED_LIBRARY)

