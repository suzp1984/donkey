LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_STATIC_LIBRARIES)
LOCAL_C_INCLUDES :=		\
	hardware/libhardware/include \
	vendor/marvell/littleton/m2d \
	vendor/marvell/generic/ipplib/include\
	$(LOCAL_PATH)/../gcu


LOCAL_SRC_FILES := mmx_copybit.c
LOCAL_MODULE := libmmx

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_STATIC_LIBRARIES := libcompsurf

include $(BUILD_STATIC_LIBRARY)

$(call add-prebuilt-files, STATIC_LIBRARIES, libcompsurf.a)
