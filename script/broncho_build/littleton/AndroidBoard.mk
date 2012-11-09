# make file for new hardware  from 
#
LOCAL_PATH := $(call my-dir)
#
# this is here to use the pre-built kernel
ifeq ($(TARGET_PREBUILT_KERNEL),)
	TARGET_PREBUILT_KERNEL := $(LOCAL_PATH)/kernel
endif
#
file := $(INSTALLED_KERNEL_TARGET)
ALL_PREBUILT += $(file)
$(file): $(TARGET_PREBUILT_KERNEL) | $(ACP)
	$(transform-prebuilt-to-target)

#init.rc for this board
file := $(TARGET_ROOT_OUT)/init.rc
$(file) : $(LOCAL_PATH)/init.rc | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(file)

#init.sh for this board
file := $(TARGET_ROOT_OUT)/init.sh
$(file) : $(LOCAL_PATH)/init.sh | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(file)

file := $(TARGET_ROOT_OUT)/init.marvell.rc
$(file) : $(LOCAL_PATH)/init.marvell.rc | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(file)

file := $(TARGET_ROOT_OUT)/init.mtk_gsm.rc
$(file) : $(LOCAL_PATH)/init.mtk_gsm.rc | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(file)

file := $(TARGET_ROOT_OUT)/init.qsc_evdo.rc
$(file) : $(LOCAL_PATH)/init.qsc_evdo.rc | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(file)

file := $(TARGET_ROOT_OUT)/init.recovery.rc
$(file) : $(LOCAL_PATH)/init.recovery.rc | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(file)

#file := $(TARGET_OUT_KEYLAYOUT)/pxa27x-keypad.kl
#ALL_PREBUILT += $(file)
#$(file): $(LOCAL_PATH)/pxa27x-keypad.kl | $(ACP)
#	$(transform-prebuilt-to-target)

#
# no boot loader, so we don't need any of that stuff..  
#

#include $(CLEAR_VARS)
#LOCAL_SRC_FILES := pxa27x-keypad.kcm
#include $(BUILD_KEY_CHAR_MAP)

#
# include more board specific stuff here? Such as Audio parameters.      
#
L_PATH := $(LOCAL_PATH)
include $(L_PATH)/wifi/Mdroid.mk
include $(L_PATH)/boot_bin/Mdroid.mk
#include $(L_PATH)/../external/alsa/Mdroid.mk
#include $(L_PATH)/libaudio/Mdroid.mk
#include $(L_PATH)/libcamera/Mdroid.mk
include $(L_PATH)/../external/tools/Mdroid.mk
include $(L_PATH)/../generic/ipplib/omx/il/Mdroid.mk
include $(L_PATH)/../generic/ipplib/lib/arm-gcc/openmax/lib/Mdroid.mk
include $(L_PATH)/../generic/ipplib/lib/arm-gcc/codec/lib/Mdroid.mk
include $(L_PATH)/mved/Mdroid.mk
include $(L_PATH)/bmm/Mdroid.mk
include $(L_PATH)/../generic/ipplib/misc/Mdroid.mk
include $(L_PATH)/overlay2display/Mdroid.mk
include $(L_PATH)/m2d/Mdroid.mk
include $(L_PATH)/broncho/Mdroid.mk
