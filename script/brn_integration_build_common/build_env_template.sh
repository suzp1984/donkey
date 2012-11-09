####################################################
if [ BRN_VERSION = "dailybuild" ];then
    export BRN_PRODUCT_NAME=A1
    export BRN_PRODUCT_MODEL=A1
    export BRN_PRODUCT_DEVICE=A1
    export BRN_PRODUCT_BRAND=broncho
else
    export BRN_PRODUCT_NAME=@name@
    export BRN_PRODUCT_MODEL=@mode@
    export BRN_PRODUCT_DEVICE=@device@
    export BRN_PRODUCT_BRAND=@brand@
fi
#export BRN_RIVERSION=`LANG=en_US.UTF-8;svn info svn+ssh://svn@192.168.1.168/donut/dev|grep Revision:|sed -e "s/Revision: /r/g"`
####################################################
export TARGET_PRODUCT=@target_product@
export TARGET_BUILD_VARIANT=user
export TARGET_SIMULATOR=false
export TARGET_BUILD_TYPE=release
export TARGET_ARCH=arm
export HOST_ARCH=x86
export HOST_OS=linux
export HOST_BUILD_TYPE=release
export BUILD_NUMBER=$TARGET_BUILD_VARIANT.@customer@.`date +%Y%m%d.%H%M%S`
export ANDROID_BUILD_TOP=`dirname $PWD`
export ANDROID_QTOOLS=$ANDROID_BUILD_TOP/development/emulator/qtools
export ANDROID_PRODUCT_OUT=$ANDROID_BUILD_TOP/out/target/product/$TARGET_PRODUCT
export BRONCHO_BUILD_TOP=$ANDROID_BUILD_TOP/integration/broncho_build
export BRONCHO_DEVICE_TOP=$ANDROID_BUILD_TOP/@device_path@
export KERNEL_VERSION=@kernel@
export BRONCHO_INTEGRATION_TOP=$ANDROID_BUILD_TOP/integration/
#export BOOTLOADER_DIR=$ANDROID_BUILD_TOP/@bootloader@
#export KERNEL_DIR=$ANDROID_BUILD_TOP/$KERNEL_VERSION
export BOOTLOADER=@bootloader@
export FLASH_SIZE=@flash_size@
export CPU_TYPE=@cpu_type@
#export TARGET_PRODUCT_DEVICE=$TARGET_PRODUCT\_$CPU_TYPE\_$FLASH_SIZE
export BRONCHO_INTEGRATION_TARGET_PRODUCT=$BRONCHO_INTEGRATION_TOP/target_out/$DEVICE_NAME
###################
export BRN_RIVERSION=`cd ${ANDROID_BUILD_TOP} && LANG=en_US.UTF-8;svn info | grep Revision:|sed -e "s/Revision: /r/g"`

echo "TARGET_PRODUCT is not set, use default:"
echo "============================================"
echo TARGET_PRODUCT=$TARGET_PRODUCT
echo TARGET_BUILD_VARIANT=$TARGET_BUILD_VARIANT
echo TARGET_SIMULATOR=$TARGET_SIMULATOR
echo TARGET_BUILD_TYPE=$TARGET_BUILD_TYPE
echo TARGET_ARCH=$TARGET_ARCH
echo HOST_ARCH=$HOST_ARCH
echo HOST_OS=$HOST_OS
echo HOST_BUILD_TYPE=$HOST_BUILD_TYPE
echo ANDROID_BUILD_TOP=$ANDROID_BUILD_TOP
echo ANDROID_QTOOLS=$ANDROID_QTOOLS
echo ANDROID_PRODUCT_OUT=$ANDROID_PRODUCT_OUT
echo ANDROID_TOOLCHAIN=$ANDROID_TOOLCHAIN
echo ANDROID_EABI_TOOLCHAIN=$ANDROID_EABI_TOOLCHAIN
echo BUILD_NUMBER=$BUILD_NUMBER
echo BRN_VERSION=$BRN_VERSION
echo BRN_PRODUCT_NAME=$BRN_PRODUCT_NAME
echo BRN_PRODUCT_MODEL=$BRN_PRODUCT_MODEL
echo BRN_PRODUCT_DEVICE=$BRN_PRODUCT_DEVICE
echo BRN_PRODUCT_BRAND=$BRN_PRODUCT_BRAND
echo BRN_RIVERSION=$BRN_RIVERSION
echo "============================================"

function brn_mkdir()
{
	if [ "x$1" != "x" ]; then
		if [ ! -d $1 ]; then
			mkdir -p  $1
		fi
	fi
}

function brn_rmdir()
{
	if [ "x$1" != "x" ]; then
		if [ -d $1 ]; then
			rm -rf $1
		fi
	fi
}

function brn_mvdir()
{
	brn_rmdir $2
	mv $1 $2
}

function brn_cpdir()
{
	brn_rmdir $2
	cp -r $1 $2
}

function brn_cpfile()
{
	if [ -f $1 ]; then
		cp -r $1 $2
	fi
}

function EXIT_IF_FAIL()
{
	if [ "$?" != "0" ]; then
		echo "  [ERROR MESSAGE]: $1"
		exit 1
	fi
}

function EXIT()
{
	echo "	[ERROR MESSAGE]: $1"
	exit 1
}

#exprot buidin function
export -f brn_mkdir
export -f brn_rmdir
export -f brn_mvdir
export -f brn_cpdir
export -f brn_cpfile
export -f EXIT_IF_FAIL
export -f EXIT
