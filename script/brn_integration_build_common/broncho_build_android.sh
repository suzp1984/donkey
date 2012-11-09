#!/bin/bash
#

set -x

echo "build android begin: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time

brn_rmdir $ANDROID_PRODUCT_OUT/system
brn_rmdir $ANDROID_PRODUCT_OUT/root

if [ -f ${BRONCHO_DEVICE_TOP}/device/before_android_build.sh ]; then
    sh ${BRONCHO_DEVICE_TOP}/device/before_android_build.sh
    EXIT_IF_FAIL "Error when exec before_android_build.sh"
fi

cd $ANDROID_BUILD_TOP
make -j2
EXIT_IF_FAIL "Error when compile android"

if [ -f ${BRONCHO_DEVICE_TOP}/device/after_android_build.sh ]; then
    sh ${BRONCHO_DEVICE_TOP}/device/after_android_build.sh
    EXIT_IF_FAIL "Error when exec after_android_build.sh"
fi

brn_mkdir $BRONCHO_INTEGRATION_TARGET_PRODUCT
brn_cpdir $ANDROID_PRODUCT_OUT/system $BRONCHO_INTEGRATION_TARGET_PRODUCT/system
brn_cpdir $ANDROID_PRODUCT_OUT/root $BRONCHO_INTEGRATION_TARGET_PRODUCT/root

#clean system.bin
sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_clean.sh system	

for item in $CUSTOMER_LIST; do
    export CUSTOMER=$item
    sh $ANDROID_BUILD_TOP/customer/$CUSTOMER/build_image.sh
	EXIT_IF_FAIL "Error when compile customer"
    $ANDROID_BUILD_TOP/out/host/linux-x86/bin/mkyaffs2image $ANDROID_PRODUCT_OUT/system $BRONCHO_INTEGRATION_TARGET_PRODUCT/system-$CUSTOMER.bin

    brn_mvdir  $ANDROID_PRODUCT_OUT/system $BRONCHO_INTEGRATION_TARGET_PRODUCT/system-$CUSTOMER
    brn_cpdir $BRONCHO_INTEGRATION_TARGET_PRODUCT/system $ANDROID_PRODUCT_OUT/system
    #brn_mvdir $BRONCHO_INTEGRATION_TARGET_PRODUCT/system.common $BRONCHO_INTEGRATION_TARGET_PRODUCT/system

    brn_cpdir $BRONCHO_INTEGRATION_TARGET_PRODUCT/root $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER
done

echo "build android end: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time

