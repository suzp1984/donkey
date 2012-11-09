#!/bin/bash

#cp $BRONCHO_DEVICE_TOP/device/buildspec.mk $ANDROID_BUILD_TOP || EXIT "cp buildspec.mk problem"

if [ -f $BRONCHO_DEVICE_TOP/device/buildspec.mk ]; then
	sed -e 's/@out_dir@/'"$out_dir"'/g' $BRONCHO_DEVICE_TOP/device/buildspec.mk > $ANDROID_BUILD_TOP/buildspec.mk 
fi

