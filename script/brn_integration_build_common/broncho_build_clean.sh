#!/bin/bash
#
function brn_clean_module()
{
	for item in $CUSTOMER_LIST; do
		if [ -f $BRONCHO_INTEGRATION_TARGET_PRODUCT/$item/$1 ]; then
			rm -rf $BRONCHO_INTEGRATION_TARGET_PRODUCT/$item/$1
		fi
	done
}

#TODO: clean BRONCHO_INTEGRATION_TARGET_PRODUCT
if [ "x$1" = "x" ]; then
	if [ -d $BRONCHO_INTEGRATION_TARGET_PRODUCT ]; then
		cd $BRONCHO_INTEGRATION_TARGET_PRODUCT

		if [ -d bootloader ]; then
			rm -r  bootloader
		fi

		if [ -d factory ]; then
			rm -r factory
		fi

		if [ -d root ]; then
			rm -r root
		fi

		if [ -d update_tools ]; then
			rm -rf update_tools
		fi

		for item in `ls  *.zip *.gz *.bin update_md5*`
		do
			rm $item
		done

		for item in `ls -d system* root*`
		do
			rm -rf $item
		done
	fi
elif [ $1 = "system" -a "x$CUSTOMER_LIST" != "x" ]; then
	brn_clean_module system.bin
elif [ $1 = "root" -a "x$CUSTOMER_LIST" != "x" ]; then
	brn_clean_module root.bin
elif [ $1 = "factory" -a "x$CUSTOMER_LIST" != "x" ]; then
	brn_clean_module factory.bin
elif [ $1 = "kernel" -a "x$CUSTOMER_LIST" != "x" ]; then
	brn_clean_module kernel.bin
	brn_clean_module recovery.bin
elif [ $1 = "update" -a "x$CUSTOMER_LIST" != "x" ]; then
	brn_clean_module update.zip
else
	echo "sorry!"
fi


