#!/bin/bash
#

# build bootloader first, if needed
if [ ! -d $BRONCHO_INTEGRATION_TARGET_PRODUCT/bootloader/ ]; then
	sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_bootloader.sh
fi

# build android first, if needed
if [ ! -d $BRONCHO_INTEGRATION_TARGET_PRODUCT/system ]; then
	sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_android.sh
fi

# build kernel first, if needed
ls $BRONCHO_INTEGRATION_TARGET_PRODUCT/kernel-*.bin > /dev/null 2>&1
#if [ $? != 0 -a ! -f $BRONCHO_INTEGRATION_TARGET_PRODUCT/kernel.bin ]; then
if [ $? != 0 ]; then
	sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_kernel.sh
fi

echo "build release begin: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time

RELEASE_NOTE=`$PYTHON_PARSER -g $DEVICE_NAME release_notes`
CUSTOMER_TO_RELEASE=""
for item in $CUSTOMER_LIST; do
	export CUSTOMER=$item
	brn_mkdir $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER 
	#copy system
	brn_cpfile $BRONCHO_INTEGRATION_TARGET_PRODUCT/system-$CUSTOMER.bin $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER/system.bin
	#copy kernel
	brn_cpfile $BRONCHO_INTEGRATION_TARGET_PRODUCT/kernel-$CUSTOMER.bin $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER/kernel.bin
	#copy recovery
	brn_cpfile $BRONCHO_INTEGRATION_TARGET_PRODUCT/recovery-$CUSTOMER.bin $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER/recovery.bin
	#copy factory
	brn_cpfile $BRONCHO_INTEGRATION_TARGET_PRODUCT/factory.bin $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER/factory.bin
	#copy rootfs
	brn_cpfile $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER.bin $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER/root.bin
	brn_cpfile $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER.tar.gz $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER/root.tar.gz
	#copy release note
	brn_cpfile $BRONCHO_INTEGRATION_TOP/$RELEASE_NOTE $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER/
	#copy update
	brn_cpfile $BRONCHO_INTEGRATION_TARGET_PRODUCT/update-$CUSTOMER.zip $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER/update.zip
	brn_cpfile $BRONCHO_INTEGRATION_TARGET_PRODUCT/update_md5-$CUSTOMER $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER/update_md5

	cd $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER 
	cp ../bootloader/* ./

	if [ "$BOOTLOADER" = "blob-wtptp" -o "$BOOTLOADER" = "blob-wtptp.new" ]; then
        cp $BRONCHO_DEVICE_TOP/device/BronchoDownloadConfig.txt .
        rm tim.bin
		./ntbb.exe -r $BRONCHO_DEVICE_TOP/device/descr_v3_lb_mhnlv.txt 
		rm -f *_h.bin
	fi
	CUSTOMER_TO_RELEASE+="$DEVICE_NAME/$CUSTOMER "
done

## add download tools for windows os
CUSTOMER_TO_RELEASE+="../tools/ "

tar -zcvf $BRONCHO_INTEGRATION_TOP/$BRN_PROJECT_NAME\_$DEVICE_NAME\_$BRN_RIVERSION\_$BRN_VERSION.tar.gz -C $BRONCHO_INTEGRATION_TOP/target_out $CUSTOMER_TO_RELEASE

md5sum  $BRONCHO_INTEGRATION_TOP/$BRN_PROJECT_NAME\_$DEVICE_NAME\_$BRN_RIVERSION\_$BRN_VERSION.tar.gz > $BRONCHO_INTEGRATION_TOP/$BRN_PROJECT_NAME\_$DEVICE_NAME\_$BRN_RIVERSION\_$BRN_VERSION\_md5

echo "build release end: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time

####################
