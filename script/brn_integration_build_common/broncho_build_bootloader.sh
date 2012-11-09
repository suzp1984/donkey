#!/bin/bash
#

set -x
echo "build bootloader begin: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time

bootloader_makefile=`$PYTHON_PARSER -g $DEVICE_NAME bootloader_makefile`

cd $ANDROID_BUILD_TOP && svn co svn+ssh://svn@192.168.1.168/all/bootloader/$BOOTLOADER
datasafe_tags set $ANDROID_BUILD_TOP/$BOOTLOADER

echo "checkout bootloader done."

cd $ANDROID_BUILD_TOP/$BOOTLOADER
unset TARGET_ARCH && make -f $bootloader_makefile

brn_mkdir $BRONCHO_INTEGRATION_TARGET_PRODUCT/bootloader

if [ "$BOOTLOADER" = "blob-wtptp" -o "$BOOTLOADER" = "blob-wtptp.new" ]; then
	cp -v target/host-windows/MHLV_NTDKB.bin $BRONCHO_INTEGRATION_TARGET_PRODUCT/bootloader 
	cp -v target/host-windows/MH_LV_DKBNTIM.bin $BRONCHO_INTEGRATION_TARGET_PRODUCT/bootloader 
	cp -v target/host-windows/BronchoDKBConfig.txt $BRONCHO_INTEGRATION_TARGET_PRODUCT/bootloader
	cp -v target/host-windows/ntbb.exe $BRONCHO_INTEGRATION_TARGET_PRODUCT/bootloader 
	cp -v target/host-windows/MHLV_linux_NTOBM.bin $BRONCHO_INTEGRATION_TARGET_PRODUCT/bootloader 
	cp -v target/host-windows/blob $BRONCHO_INTEGRATION_TARGET_PRODUCT/bootloader
fi

echo "build bootloader end: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time


