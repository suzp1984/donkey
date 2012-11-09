#!/bin/bash
#
# factory-files.txt must be provied by vender-device

# compile android first, if needed
if [ ! -d $BRONCHO_INTEGRATION_TARGET_PRODUCT/system ]; then
	sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_android.sh
fi

set -x

sh $BRONCHO_INTEGRATION_TOP/broncho_build/broncho_build_clean.sh factory

echo "build factory begin: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time

brn_mkdir $BRONCHO_INTEGRATION_TARGET_PRODUCT

cd $BRONCHO_INTEGRATION_TARGET_PRODUCT

if [ -d ./factory ]; then
	rm -rf factory
fi

mkdir -p factory

for file in `cat $BRONCHO_DEVICE_TOP/device/factory-files.txt`
do
	if [ -d $BRONCHO_INTEGRATION_TARGET_PRODUCT/$file ]; then
		mkdir -p  factory/$file
	else
		cp -dpv $BRONCHO_INTEGRATION_TARGET_PRODUCT/$file factory/$file
	fi
done

if [ -x /sbin/mkfs.cramfs ]; then 
	/sbin/mkfs.cramfs factory/system factory.bin 
else 
	echo ""
	echo "" 
	echo "Not found /sbin/mkfs.cramfs";
	exit 1 
fi

echo "build factory end: " >> $BRONCHO_INTEGRATION_TOP/compile_time
date +%H:%M:%S >> $BRONCHO_INTEGRATION_TOP/compile_time

