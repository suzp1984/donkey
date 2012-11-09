LOCAL_PATH := $(call my-dir)

# add broncho hotplug
#include $(CLEAR_VARS)
#
#file := $(TARGET_ROOT_OUT)/sbin/hotplug
#$(file) : $(LOCAL_PATH)/root/sbin/hotplug | $(ACP)
#	$(transform-prebuilt-to-target)
#
#ALL_PREBUILT += $(file)

#use broncho logo
#include $(CLEAR_VARS)
#
#file := $(TARGET_ROOT_OUT)/initlogo.rle
#$(file) : $(LOCAL_PATH)/root/logo.rle | $(ACP)
#	$(transform-prebuilt-to-target)
#
#ALL_PREBUILT += $(file)

# use broncho busybox
include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/busybox
$(file) : $(LOCAL_PATH)/system/bin/busybox | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/cp
$(file) : $(LOCAL_PATH)/system/bin/cp | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/head
$(file) : $(LOCAL_PATH)/system/bin/head | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/killall
$(file) : $(LOCAL_PATH)/system/bin/killall | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/md5sum
$(file) : $(LOCAL_PATH)/system/bin/md5sum | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

# bluetooth staff
#include $(CLEAR_VARS)
#file := $(TARGET_OUT)/bin/bt_init
#$(file) : $(LOCAL_PATH)/system/bin/bt_init | $(ACP)
#	mkdir -p $(dir $@)
#	$(ACP) -dfpv $< $@
#ALL_PREBUILT += $(file)
#
#include $(CLEAR_VARS)
#file := $(TARGET_OUT)/bin/bt_deinit
#$(file) : $(LOCAL_PATH)/system/bin/bt_deinit | $(ACP)
#	mkdir -p $(dir $@)
#	$(ACP) -dfpv $< $@
#ALL_PREBUILT += $(file)
#
#include $(CLEAR_VARS)
#file := $(TARGET_OUT)/xbin/bccmd
#$(file) : $(LOCAL_PATH)/system/xbin/bccmd | $(ACP)
#	mkdir -p $(dir $@)
#	$(ACP) -dfpv $< $@
#ALL_PREBUILT += $(file)

#include $(CLEAR_VARS)
#file := $(TARGET_OUT)/etc/bluetooth/csr.psr
#$(file) : $(LOCAL_PATH)/system/etc/bluetooth/csr.psr | $(ACP)
#	mkdir -p $(dir $@)
#	$(ACP) -dfpv $< $@
#ALL_PREBUILT += $(file)

# wifi staff
include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/wlan_loader
$(file) : $(LOCAL_PATH)/system/bin/wlan_loader | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/wifi_init
$(file) : $(LOCAL_PATH)/system/bin/wifi_init | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/wifi_deinit
$(file) : $(LOCAL_PATH)/system/bin/wifi_deinit | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/gh3801_init
$(file) : $(LOCAL_PATH)/system/bin/gh3801_init | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/iwlist
$(file) : $(LOCAL_PATH)/system/bin/iwlist | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/iwpriv
$(file) : $(LOCAL_PATH)/system/bin/iwpriv | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/iwconfig
$(file) : $(LOCAL_PATH)/system/bin/iwconfig | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/lib/libiw.so
$(file) : $(LOCAL_PATH)/system/lib/libiw.so | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

# gprs staff
include $(CLEAR_VARS)
file := $(TARGET_OUT)/etc/ppp/ip-up
$(file) : $(LOCAL_PATH)/system/etc/ppp/ip-up | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/etc/ppp/ip-down
$(file) : $(LOCAL_PATH)/system/etc/ppp/ip-down | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/xbin/pppd_gprs
$(file) : $(LOCAL_PATH)/system/xbin/pppd_gprs | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

# install prebuild media
#include $(CLEAR_VARS)
#LOCAL_MEDIA_DIR := $(LOCAL_PATH)/system/media

#define local_find_media_files
#$(patsubst ./%,%,$(shell cd $(LOCAL_PATH)/system/media; find . -type f -printf "%P\n" | grep -v '.svn'))
#endef

#copy_from := $(call local_find_media_files)
#copy_to   := $(addprefix $(TARGET_OUT)/media/,$(copy_from))
#copy_from := $(addprefix $(LOCAL_MEDIA_DIR)/,$(copy_from))

#$(copy_to) : $(TARGET_OUT)/media/% : $(LOCAL_MEDIA_DIR)/% | $(ACP)
#	$(transform-prebuilt-to-target)
#ALL_PREBUILT += $(copy_to)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/system_check
$(file) : $(LOCAL_PATH)/system/bin/system_check | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/system_adc
$(file) : $(LOCAL_PATH)/system/bin/system_adc | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/system_imei
$(file) : $(LOCAL_PATH)/system/bin/system_imei | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)

include $(CLEAR_VARS)
file := $(TARGET_OUT)/bin/system_sn
$(file) : $(LOCAL_PATH)/system/bin/system_sn | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) -dfpv $< $@
ALL_PREBUILT += $(file)
