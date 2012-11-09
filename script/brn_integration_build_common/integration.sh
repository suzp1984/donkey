#!/bin/bash
#
# TODO: 1. parser the cmd line 
CWD=$PWD
export PYTHON_PARSER="$PWD/tools/BronchoParser.py -f $PWD/../config"
export PYTHON_DIR_WALK="$PWD/tools/oswalk.py"

DEVICE_LIST=`$PYTHON_PARSER -d`
MODULES_LIST=""
TMP_MODULES_LIST=`$PYTHON_PARSER -m`
ORDER_MODULES="android rootfs kernel bootloader factory update release clean"


usage() {
	echo ""
	echo "=============================================="
	echo "Usage:  [version number] [[-l] [[-d \"devices\"] [-m \"modules\"] ]] "
	echo "	-l List the available devices and modules" 
	echo "	-d \"devices list\" compile the choose devices"
	echo "		[default settings in config file]"
	echo "	-m \"modules list\" compile the choose modules"
	echo "		[default settings in config file]"
	echo "==============================================="
	echo 
}

warning() {
	echo ""
	echo "============================================"
	echo "warning message:"
	echo "	$1"
	echo ""
	echo "try -l option, list all available devies and modules"
	echo "============================================"
	echo ""
}

list_option() {
		$PYTHON_PARSER -l
		echo
		echo "	default_devices: $DEVICE_LIST"
		echo
		echo  "  [the avilable modules] "
		echo
		for item in ./broncho_build_*.sh
		do
			filename=`basename $item`
			modulename=`echo $filename | sed -e 's/broncho_build_//' -e 's/\.sh//'`
			echo "	$modulename "
		done
		echo
		echo "	defaut_modules: $MODULES_LIST "
		echo
		echo "  [the avilable customers] "
		echo
		for item in `ls -d ../../customer/*`
		do
			customer_name=`basename $item`
			echo "	$customer_name  "
		done
		echo
		echo
}

check_device_list() {
	for item in $DEVICE_LIST; do
		judgement=`$PYTHON_PARSER -j $item`
		if [ $judgement = 'false' ]; then
			warning "$1 Invalid input: do not have device: $item"
			exit 1
		fi
	done
}

check_module_list() {
	MODULES_LIST=""
	for order in $ORDER_MODULES; do
		for item in $TMP_MODULES_LIST; do
			if [ ! -f ./broncho_build_$item.sh ]; then
				warning "$1 Invalid input: do not have the module: $item"
				exit 1
			fi

			if [ "$item" = "$order" ]; then
				MODULES_LIST+=" $order"
			fi
		done
	done
}

check_device_list  "config file"
check_module_list  "config file"

if [ "$1" = "" -o "$1" = "-d"  -o "$1" = "-m" ]
then
	echo "Usage $0 [version(0.6)]"
	export BRN_VERSION="dailybuild"
	echo "set BRN_VERSION to $BRN_VERSION as daily build."
elif [ "$1" = "-h" -o "$1" = "--help" ]; then
	usage
	exit 0
elif [ "$1" = "-l" ]; then
	list_option
	exit 0
else
	export BRN_VERSION=$1
#	if [ -d ../target_out ]; then
#		rm -rf ../target_out
#	fi
	shift
fi

while [ "$1" != "" ]
do
	if [ "$1" = "-l" ]; then
		list_option
		exit 0
	elif [ "$1" = "-d" ]; then
		shift
		DEVICE_LIST=$1
		check_device_list "input cmd"
	elif [ "$1" = "-m" ]; then
		shift
		TMP_MODULES_LIST=$1
		check_module_list "input cmd"
	elif [ "$1" = "-h" ]; then
		usage
		exit 0
	else
		shift
	fi
done

echo
echo "================================="
echo -e "\t [the devices list]: $DEVICE_LIST"
echo -e "\t [the modules list]: $MODULES_LIST"
echo "================================="
echo

# 2. generate build_env.sh file 
# 3. for loop: compile the devices
for device_item in $DEVICE_LIST
do
	export DEVICE_NAME=$device_item
	kernel_version=`$PYTHON_PARSER -g $device_item kernel`
	device_path=`$PYTHON_PARSER -g $device_item device_path`
	flash_size=`$PYTHON_PARSER -g $device_item flash_size`
	target_product=`$PYTHON_PARSER -g $device_item target_product`
	export cpu_type=`$PYTHON_PARSER -g $device_item cpu_type`
	bootloader=`$PYTHON_PARSER -g $device_item bootloader`
	export CUSTOMER_LIST=`$PYTHON_PARSER -g $device_item customers`

	for customer_item in $CUSTOMER_LIST; do
		if [ ! -d ../../customer/$customer_item ]; then
			warning "Invalid config customer option: $customer_item"
			exit 1
		fi
	done

	sed -e 's/@kernel@/'"$kernel_version"'/g' -e 's#@device_path@#'"$device_path"'#g' -e 's/@flash_size@/'"$flash_size"'/g' -e 's/@target_product@/'"$target_product"'/g'  -e 's/@cpu_type@/'"$cpu_type"'/g' -e 's/@bootloader@/'"$bootloader"'/g' build_env_template.sh > ../build_env.sh

	# Setting env
	cd ../../ && . ./build/envsetup.sh
	cd $CWD/../ && . ./build_project_env.sh && . ./build_env.sh
	cd $CWD
	# do in sequence of MODULES_LIST
	for module_item in $MODULES_LIST
	do
		echo $PWD
		cd $CWD
		sh ./broncho_build_$module_item.sh
		EXIT_IF_FAIL "build android module $module_item Error"	
		
	done
done
