LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := vendor/marvell/generic/include

LOCAL_SRC_FILES:= \
	m2d_lib.c gcu.c

LOCAL_MODULE:= libMrvlm2d
LOCAL_PRELINK_MODULE := false
LOCAL_STRIP_MODULE := false

include $(BUILD_SHARED_LIBRARY)
