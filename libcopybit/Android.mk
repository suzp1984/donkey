# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.board.platform>.so

LOCAL_PATH:= $(call my-dir)
MMX_PATH:= $(LOCAL_PATH)/mmx
#GCU_PATH:= $(LOCAL_PATH)/gcu
STATS_PATH:= $(LOCAL_PATH)/stats
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_C_INCLUDES :=		\
	hardware/libhardware/include	\
	vendor/marvell/littleton/m2d	\
	vendor/marvell/generic/ipplib/include \
#	$(GCU_PATH)


LOCAL_SRC_FILES := copybit.c
LOCAL_MODULE := copybit.marvell

LOCAL_SHARED_LIBRARIES := libMrvlm2d liblog
LOCAL_STATIC_LIBRARIES := libmmx libcompsurf 

include $(BUILD_SHARED_LIBRARY)
include $(MMX_PATH)/Mdroid.mk
#include $(GCU_PATH)/Mdroid.mk
include $(STATS_PATH)/Mdroid.mk

