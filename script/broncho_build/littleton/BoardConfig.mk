# config.mk
#
# Product-specific compile-time definitions.
#

# The generic product target doesn't have any hardware-specific pieces.
TARGET_CPU_ABI := armeabi
TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := true
TARGET_NO_RADIOIMAGE := false
HAVE_HTC_AUDIO_DRIVER := false
BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_ALSA_AUDIO := true
BUILD_WITH_ALSA_UTILS := true
USE_CAMERA_STUB := true
BOARD_HAVE_BLUETOOTH := true
TARGET_PROVIDES_INIT_RC := true
USE_MARVELL_BOARD := true
USE_MARVELL_MVED := false
USE_MARVELL_OVERLAY2 := true
USE_MARVELL_GCU := false
USE_MARVELL_IPP_OPENMAX := true
USE_MARVELL_IPP_CODEC := true
BOARD_WPA_SUPPLICANT_DRIVER := MARVELL 
WIFI_DRIVER_MODULE_PATH := /system/lib/modules/sd8688.ko
WIFI_DRIVER_MODULE_NAME := sd8xxx
BOARD_HAVE_BRONCHO_GPS := true
PRODUCT_LOCALES=zh_CN
