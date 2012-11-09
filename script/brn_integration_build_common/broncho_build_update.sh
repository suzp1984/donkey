#!/bin/bash
#

ls $BRONCHO_INTEGRATION_TARGET_PRODUCT/system-* > /dev/null 2>&1
if [ $? != 0 ]; then
	sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_android.sh
fi

set -x
echo "build update begin: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time

brn_mkdir $BRONCHO_INTEGRATION_TARGET_PRODUCT

cd $BRONCHO_INTEGRATION_TARGET_PRODUCT

echo "============================================"
echo "build_customer_update_package()"
echo "TARGET=$TARGET_PRODUCT"
echo "============================================"

brn_rmdir update_tools 
cp -r $ANDROID_BUILD_TOP/external/update_tools . 
# TODO: no updater in out dir
cp $BRONCHO_INTEGRATION_TARGET_PRODUCT/system/bin/updater update_tools/a1/META-INF/com/google/android/update-binary || EXIT "DID NOT find updater file in /system/bin!!"

for item in $CUSTOMER_LIST;  do
	export CUSTOMER=$item
	echo "Build package for target--$DEVICE_NAME, customer--$CUSTOMER"

	#update customer kernel
	rm -f ./update_tools/a1/boot.img
	echo `pwd`
	cp kernel-$CUSTOMER.bin update_tools/a1/boot.img 

	#update customer system
	brn_cpdir  $BRONCHO_INTEGRATION_TARGET_PRODUCT/system-$CUSTOMER ./update_tools/a1/system

	cd update_tools/ && ./build.sh a1 > /dev/null && cd -
	cp update_tools/update.zip $BRONCHO_INTEGRATION_TARGET_PRODUCT/update-$CUSTOMER.zip

	md5sum update_tools/update.zip > $BRONCHO_INTEGRATION_TARGET_PRODUCT/update_md5-$CUSTOMER
done


echo "build update end: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time
