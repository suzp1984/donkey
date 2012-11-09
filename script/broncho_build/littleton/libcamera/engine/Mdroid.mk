LOCAL_PATH:= $(call my-dir)

CE_LIB_PATH := lib
CE_SRC_PATH := src
CE_INC_PATH := include

#
# libcamerahal
#

include $(CLEAR_VARS)

# put your source files here.
LOCAL_SRC_FILES:=                       \
    $(CE_SRC_PATH)/subcomp_stub.c    	\
    $(CE_SRC_PATH)/sensor_stub.c

# put the libraries you depend on here.
    
LOCAL_CFLAGS +=                             \
    -I $(LOCAL_PATH)/$(CE_INC_PATH)         \
    -D ANDROID_CAMERAENGINE                 \
    -D CAM_LOG_VERBOSE 
#    -mabi=aapcs-linux

# if you have any extra directories, put them here.
LOCAL_C_INCLUDES := \

# put your module name here
LOCAL_MODULE:= libcamerahal	

include $(BUILD_STATIC_LIBRARY)

include $(LOCAL_PATH)/$(CE_LIB_PATH)/Mdroid.mk
