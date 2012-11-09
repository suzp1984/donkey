#!/bin/bash
echo "$0 begin"
echo "CUSTOMER=$CUSTOMER"
echo "ANDROID_BUILD_TOP=$ANDROID_BUILD_TOP"
echo "ANDROID_PRODUCT_OUT=$ANDROID_PRODUCT_OUT"
echo "Broncho A1 for $CUSTOMER" > $ANDROID_PRODUCT_OUT/system/etc/customer

. $ANDROID_BUILD_TOP/build/envsetup.sh

ACP=$ANDROID_BUILD_TOP/out/host/linux-x86/bin/acp

#update resouces
if [ -f $ANDROID_BUILD_TOP/customer/$CUSTOMER/resources.zip ]; then
    unzip -o $ANDROID_BUILD_TOP/customer/$CUSTOMER/resources.zip -d $ANDROID_BUILD_TOP/
    cd $ANDROID_BUILD_TOP
    mmm frameworks/base/core/res -B
    mmm packages/apps/Launcher2 -B
    mmm packages/apps/Settings -B
    cd -
fi

#remove eng packages
REMOVE_APPS="PowerMonitor.apk
    SpareParts.apk
    Development.apk"
for app in $REMOVE_APPS;
do
	rm -rfv $ANDROID_PRODUCT_OUT/system/app/$app
done

#install packages.
PREINSTALL_APPS="GPSStatus2b2.apk linpack.apk"

for app in $PREINSTALL_APPS;
do
	$ACP -dfpv $ANDROID_BUILD_TOP/preinstall/system/app/$app $ANDROID_PRODUCT_OUT/system/app/$app
done

. $ANDROID_BUILD_TOP/preinstall/google/install.sh
google_install_all

BRN_BUILD_PROP_DISPLAY_ID=$BRN_PRODUCT_NAME-$BRN_RIVERSION-$TARGET_BUILD_VARIANT\ $BUILD_ID\.$BUILD_NUMBER
sed -i -e 's/^ro.build.display.id=.*$/ro.build.display.id='"$BRN_BUILD_PROP_DISPLAY_ID"'/g' $ANDROID_PRODUCT_OUT/system/build.prop
sed -i -e 's/@customer@/'"$CUSTOMER"'/g' -e 's/@name@/A1/g' -e 's/@mode@/A1/g' -e 's/@device@/A1/g' -e 's/@brand@/broncho/g'  $ANDROID_PRODUCT_OUT/system/build.prop 

cp $ANDROID_BUILD_TOP/customer/$CUSTOMER/apns-conf.xml $ANDROID_PRODUCT_OUT/system/etc/apns-conf.xml

echo "$0 end"
