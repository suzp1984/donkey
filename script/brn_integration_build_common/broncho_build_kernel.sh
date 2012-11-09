#!/bin/bash
#
release_config=`$PYTHON_PARSER -g $DEVICE_NAME kernel_config`
recovery_config=`$PYTHON_PARSER -g $DEVICE_NAME recovery_kernel_config`

# if no root dir in product_out, compile the android first
if [ ! -d $BRONCHO_INTEGRATION_TARGET_PRODUCT/root ]; then
	sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_android.sh
	sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_rootfs.sh
fi

ls $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-* > /dev/null 2>&1
if [ $? != 0 ]; then
	sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_rootfs.sh
fi

set -x

sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_clean.sh kernel

echo "build kernel begin: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time

cd $ANDROID_BUILD_TOP
echo "checkout kernel $KERNEL_VERSION source"
if [ ! -d $KERNEL_VERSION ]; then
	svn co svn+ssh://svn@192.168.1.168/all/kernel/$KERNEL_VERSION
fi

datasafe_tags set $KERNEL_VERSION
cd $ANDROID_BUILD_TOP/$KERNEL_VERSION

#call deivce hook
if [ -f ${BRONCHO_DEVICE_TOP}/device/before_kernel_build.sh ]; then
    sh ${BRONCHO_DEVICE_TOP}/device/before_kernel_build.sh
fi

brn_rmdir root
brn_rmdir recovery

#Create recovery root directory
cp -frv $BRONCHO_INTEGRATION_TARGET_PRODUCT/root recovery
cp $ANDROID_PRODUCT_OUT/system/bin/recovery recovery/sbin/recovery
cp $ANDROID_PRODUCT_OUT/system/bin/busybox recovery/sbin/busybox

#Copy recovery res
mkdir -p recovery/res/images
cp $ANDROID_BUILD_TOP/bootable/recovery/res/images/*.png recovery/res/images/
cp $ANDROID_BUILD_TOP/bootable/recovery/res/keys recovery/res/keys
rm -f recovery/init.rc
rm -f recovery/init.qsc_evdo.rc
rm -f recovery/init.mtk_gsm.rc
mv recovery/init.recovery.rc recovery/init.rc
make ARCH=arm distclean


# copy the result to the integration target_out
for item in $CUSTOMER_LIST; do
	export CUSTOMER=$item
	rm -rf root
	cp -r $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER $ANDROID_BUILD_TOP/$KERNEL_VERSION/root
	
	make ARCH=arm $release_config 
	make ARCH=arm CROSS_COMPILE=arm-eabi- -j2
	EXIT_IF_FAIL "Error: Kernel compile problem"
	cp -v arch/arm/boot/zImage $BRONCHO_INTEGRATION_TARGET_PRODUCT/kernel-$CUSTOMER.bin


   	#Compile the kernel
	rm arch/arm/boot/zImage  

	sed -e "s/CONFIG_INITRAMFS_SOURCE=\"root\"/CONFIG_INITRAMFS_SOURCE=\"recovery\"/" arch/arm/configs/$release_config > arch/arm/configs/$recovery_config
	make ARCH=arm $recovery_config
	make ARCH=arm CROSS_COMPILE=arm-eabi- -j2 
	EXIT_IF_FAIL "Error: Kernel recovery mode compile problem"
	cp -v arch/arm/boot/zImage $BRONCHO_INTEGRATION_TARGET_PRODUCT/recovery-$CUSTOMER.bin

	brn_mkdir $BRONCHO_INTEGRATION_TARGET_PRODUCT/$CUSTOMER

done

echo "build kernel end: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time

