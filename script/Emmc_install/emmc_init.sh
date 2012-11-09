#!/bin/bash
error()
{
	echo ""
	echo "*********************"
	echo "* INSTALL FAILED *"
	echo "*********************"
	echo ""
	sync;sync;sync
	cd /
	umount /tmp/mmc
	exit 1
}
echo ""
echo "*********************"
echo "* INSTALL eMMC BOOT *"
echo "*********************"
cd /
echo ""
echo "---------------------"
echo "- CHECK u-boot-emmc -"
echo "---------------------"
if [ -f /tmp/sd/install/u-boot-emmc.bin ] ; then
	echo "find : /tmp/sd/install/u-boot-emmc.bin"
else
	echo "boot file not found"
	error
fi

echo ""
echo "---------------------"
echo "- CHECK uImage -"
echo "---------------------"
if [ -f /tmp/sd/install/uImage ] ; then
	echo "find : /tmp/sd/install/uImage"
else
	echo "kernel file not found"
	error
fi
echo ""
echo "---------------------"
echo "- CHECK android-fs -"
echo "---------------------"
FS_FILE=`ls /tmp/sd/install/android-fs*.tar.gz`
if [ -f /tmp/sd/install/android-fs*.tar.gz ] ; then
	echo "find : $FS_FILE"
else
	echo "android-fs file not found"
	error
fi
echo ""
echo "---------------------"
echo "-MAKE eMMC partition-"
echo "---------------------"

fdisk /dev/mmcblk0 <<EOF > /dev/null 2>&1 || error
d
1
d
2
d
3
d
4
d
n
p
1
+512K
n
p
2
+5M
n
p
3
w
EOF
if [ -b /dev/mmcblk0p1 ] ; then
	echo "find : /dev/mmcblk0p1"
else
	echo "/dev/mmcblk0p1 not found"
	error
fi
if [ -b /dev/mmcblk0p2 ] ; then
	echo "find : /dev/mmcblk0p2"
else
	echo "/dev/mmcblk0p2 not found"
	error
fi
if [ -b /dev/mmcblk0p3 ] ; then
	echo "find : /dev/mmcblk0p3"
else
	echo "/dev/mmcblk0p3 not found"
	error
fi
echo ""
echo "---------------------"
echo "- WRITE u-boot-emmc -"
echo "---------------------"
dd if=/tmp/sd/install/u-boot-emmc.bin of=/dev/mmcblk0p1 || error
echo ""
echo "---------------------"
echo "- WRITE uImage -"
echo "---------------------"
dd if=/tmp/sd/install/uImage of=/dev/mmcblk0p2 || error
echo ""
echo "---------------------"
echo "- WRITE android-fs -"
echo "---------------------"
mkfs.ext3 /dev/mmcblk0p3 || error
mkdir /tmp/mmc
mount -t ext3 /dev/mmcblk0p3 /tmp/mmc
tar zxvf $FS_FILE -C /tmp/mmc

sync;sync;sync
cd /
umount /tmp/mmc
echo ""
echo "*********************"
echo "* INSTALL COMPLETE! *"
echo "*********************"
echo ""
exit 0
