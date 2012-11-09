#!/bin/bash
#

# if no root dir in product_out, compile the android first
if [ ! -d $BRONCHO_INTEGRATION_TARGET_PRODUCT/root ]; then
	sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_android.sh
fi

sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_clean.sh rootfs

for item in $CUSTOMER_LIST; do
    export CUSTOMER=$item

	brn_cpdir  $BRONCHO_INTEGRATION_TARGET_PRODUCT/root $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER
	rm $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER/init.recovery.rc

	if [ -f $ANDROID_BUILD_TOP/customer/$CUSTOMER/build_initram.sh ]; then
		sh $ANDROID_BUILD_TOP/customer/$CUSTOMER/build_initram.sh
		EXIT_IF_FAIL "compile $CUSTOMER build_initram.sh fail"
	fi
	tar -czvf $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER.tar.gz $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER

	if [ -x /sbin/mkfs.cramfs ]; then
		/sbin/mkfs.cramfs $BRONCHO_INTEGRATION_TARGET_PRODUCT/root-$CUSTOMER $BRONCHO_INTEGRATION_TARGET_PRODUCT/rootfs-$CUSTOMER.bin
	else
		echo 
		echo
		echo "Not found /sbin/mkfs.cramfs" 
		exit 1 
	fi	
done
