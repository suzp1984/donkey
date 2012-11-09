LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#
# AudioHardwareMarvell target
#
LOCAL_SRC_FILES:= \
        AudioHardwareLittleton.cpp

LOCAL_SHARED_LIBRARIES := \
        libasound \
        libutils \
        libdl \
        libcutils

LOCAL_MODULE:= libaudiod

LOCAL_C_INCLUDES += \
	vendor/marvell/external/alsa/alsa-lib/include

LOCAL_CPPFLAGS += -mabi=aapcs-linux

LOCAL_STATIC_LIBRARIES += libaudiointerface

include $(BUILD_SHARED_LIBRARY)
