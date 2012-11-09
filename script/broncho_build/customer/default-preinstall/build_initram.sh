#!/bin/bash
#CUSTOMER=$ANDROID_BUILD_TOP/customer/$CUSTOMER
echo "PWD=$PWD"
echo "CUSTOMER=$CUSTOMER"

#may be useful
$ANDROID_BUILD_TOP/out/host/linux-x86/bin/img2rle $ANDROID_BUILD_TOP/customer/$CUSTOMER/logo.png $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER/initlogo.rle 
cat $ANDROID_BUILD_TOP/customer/$CUSTOMER/default.prop > $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER/default.prop

echo "$CUSTOMER end"
echo 
