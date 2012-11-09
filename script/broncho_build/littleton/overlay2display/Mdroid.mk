LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	display.cpp 

LOCAL_MODULE:= libMrvlOverlay

LOCAL_PRELINK_MODULE := false
LOCAL_STRIP_MODULE := false

include $(BUILD_SHARED_LIBRARY)
