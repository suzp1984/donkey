LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH), arm)
	LOCAL_CFLAGS += -DPACKED="__attribute__ ((packed))"
else
	LOCAL_CFLAGS += -DPACKED=""
endif

ifneq ($(USE_CUSTOM_RUNTIME_HEAP_MAX),)
  LOCAL_CFLAGS += -DCUSTOM_RUNTIME_HEAP_MAX=$(USE_CUSTOM_RUNTIME_HEAP_MAX)
endif

LOCAL_C_INCLUDES += \
	$(call include-path-for,bluez)/include \
	$(JNI_H_INCLUDE)

LOCAL_SHARED_LIBRARIES := \
	libnativehelper \
	libandroid_runtime \
	libutils \
	libcutils \
	libbluetooth

LOCAL_MODULE	:= libfm_jni
LOCAL_SRC_FILES := com_broncho_jnibt_FMNative.cpp \
					libfmradio.c

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
