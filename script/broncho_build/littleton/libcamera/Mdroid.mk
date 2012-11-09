LOCAL_PATH:= $(call my-dir)

#
# libcamera
#

include $(CLEAR_VARS)

# put your source files here
LOCAL_SRC_FILES:=               \
    CameraHardware.cpp

# put the libraries you depend on here.
LOCAL_SHARED_LIBRARIES:= \
    libui \
    libutils \
    liblog \
    libcutils \
    libMrvlIPP \
    libmiscGen

# put your module name here
LOCAL_MODULE:= libcamera

ifeq ($(TARGET_PRODUCT),littleton)
  LOCAL_CFLAGS +=     		\
    -D ANDROID_CAMERAENGINE \
    -I vendor/marvell/littleton/libcamera/engine/include \
    -I vendor/marvell/generic/ipplib/include \
    -I vendor/marvell/littleton/overlay2display \
    -D LITTLETON
endif

ifeq ($(TARGET_PRODUCT),tavorevb)
  LOCAL_CFLAGS +=     		\
    -D ANDROID_CAMERAENGINE \
    -I vendor/marvell/tavorevb/libcamera/engine/include \
    -I vendor/marvell/generic/ipplib/include \
    -I vendor/marvell/tavorevb/overlay2display \
    -D TAVOREVB
endif

ifeq ($(USE_MARVELL_OVERLAY2),false)
LOCAL_CFLAGS += -D NO_OVERLAY2
else
LOCAL_SHARED_LIBRARIES += libMrvlOverlay
endif

LOCAL_STATIC_LIBRARIES += libcameraengine       \
                          libcamerahal

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/engine/Mdroid.mk
