#!/bin/bash

if [ -f $ANDROID_BUILD_TOP/buildspec.mk ]; then
	rm -f $ANDROID_BUILD_TOP/buildspec.mk 
fi
