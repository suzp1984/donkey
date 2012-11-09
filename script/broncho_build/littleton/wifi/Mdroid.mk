LOCAL_PATH := $(call my-dir)

wifi_helper := $(TARGET_OUT_ETC)/firmware/mrvl/helper_sd.bin
$(wifi_helper) : $(LOCAL_PATH)/helper_sd.bin | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(wifi_helper)

wifi_firmware := $(TARGET_OUT_ETC)/firmware/mrvl/sd8688.bin
$(wifi_firmware) : $(LOCAL_PATH)/sd8688.bin | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(wifi_firmware)

wifi_module := $(TARGET_OUT)/lib/modules/sd8688.ko
$(wifi_module) : $(LOCAL_PATH)/sd8688.ko | $(ACP)
#$(wifi_module) : $(LOCAL_PATH)/sd8688_29.ko | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(wifi_module)


bt_module := $(TARGET_OUT)/lib/modules/bt8688.ko
$(bt_module) : $(LOCAL_PATH)/bt8688.ko | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(bt_module)

wifi_config := $(TARGET_OUT)/etc/wifi/wpa_supplicant.conf
$(wifi_config) : $(LOCAL_PATH)/wpa_supplicant.conf | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(wifi_config)
