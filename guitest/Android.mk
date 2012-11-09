LOCAL_PATH := $(call my-dir)

#
# input_event_recorder
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := input_event_recorder.c uinput.c input_event_player.c config_parser.c
LOCAL_CFLAGS += -DHAS_LINUX_UINPUT_H 
LOCAL_MODULE := input_event_recorder
LOCAL_STATIC_LIBRARIES := libcutils libc

include $(BUILD_EXECUTABLE)


# Make a symlink from /sbin/ueventd to /init
#TARGET_OUT_EXECUTABLES:=
SYMLINKS := $(TARGET_OUT_EXECUTABLES)/input_event_player
$(SYMLINKS): INIT_BINARY := $(LOCAL_MODULE)
$(SYMLINKS): $(LOCAL_INSTALLED_MODULE) $(LOCAL_PATH)/Android.mk
	@echo "Symlink: $@ -> ./$(INIT_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf ./$(INIT_BINARY) $@

ALL_DEFAULT_INSTALLED_MODULES += $(SYMLINKS)

# We need this so that the installed files could be picked up based on the
# local module name
ALL_MODULES.$(LOCAL_MODULE).INSTALLED := \
       $(ALL_MODULES.$(LOCAL_MODULE).INSTALLED) $(SYMLINKS)

