#!/bin/bash
# version: v3 
# release note: 
#echo $1
#echo $3
# table list of configure file
# devnum filesystem cmd filelist size devpath
devpath=
filesystem=
cmd=
filelist=
size=

# define some constant vars
INSTALL_DIR=/tmp/sd/install
PARTITION_TABLE=$INSTALL_DIR/partition_cmd
CONFIG_FILE=$INSTALL_DIR/cfg.ini
FORCE_FORMAT='n'
DEV_PATH=
DEVNUM=
PARTITION_LIST_F=$INSTALL_DIR/dev_list

function error()
{
	echo $1
	exit -1
}

function dump_partition_table()
{
	i=0
	while [ "$i" -lt "$DEVNUM" ]
	do
		i=$((i+1))
		if [ $i -eq 4 ]; then
			continue
		fi
		echo "************device $i***********"
		echo -n "RECORD_FILESYSTEM_$i="
		eval "echo \${RECORD_FILESYSTEM_${i}}"
		echo -n "RECORD_CMD_$i="
		eval "echo \${RECORD_CMD_${i}}"
		echo -n "RECORD_FILELIST_$i="
		eval "echo \${RECORD_FILELIST_${i}}"
		echo -n "RECORD_SIZE_$i="
		eval "echo \${RECORD_SIZE_${i}}"
		echo -n "RECORD_DEVPATH_$i="
		eval "echo \${RECORD_DEVPATH_${i}}"
	done
}

# step 1: check the config file.
if ! [ -f "$CONFIG_FILE" ]; then
	error "the file $1 do not exits!"
fi

# step 2: parse the config file
for line in `cat $CONFIG_FILE | tr -s ' ' | sed -e 's/ /@/g'`
do
	line=`echo $line | sed -e 's/@/ /g'`
	echo $line | grep '^DEV_PATH' > /dev/null 2>&1
	if [ $? = '0' ]; then
		DEV_PATH=`echo $line | sed 's/#.*$//g' | sed 's/ //g' | sed 's/^DEV_PATH=//g'`
		continue
	fi
	
	echo $line | grep '^FORCE_FORMAT' > /dev/null 2>&1
	if [ $? = '0' ]; then
		FORCE_FORMAT=`echo $line | sed 's/#.*$//g' | sed 's/ //g' | sed 's/^FORCE_FORMAT=//g'`
		continue
	fi

	echo $line | sed 's/ //g' | grep '^#' > /dev/null 2>&1
	if [ $? = '0' ]; then
		continue
	fi

	item_num=0
	for item in $line 
	do
		item_num=$((item_num+1))
		
		case "$item_num" in
			"1")
				DEVNUM=$item;;
			"2")
				filesystem=$item;;
			"3")
				cmd=$item;;
			"4")
				filelist=$item;;
			"5")
				size=$item;;
			"6")
				devpath=$item;;
		esac

	done

	eval "RECORD_FILESYSTEM_${DEVNUM}=$filesystem"
	eval "RECORD_CMD_${DEVNUM}=$cmd"
	eval "RECORD_FILELIST_${DEVNUM}=$filelist"
	eval "RECORD_SIZE_${DEVNUM}=$size"
	eval "RECORD_DEVPATH_${DEVNUM}=$devpath"
	
	if [ $DEVNUM -lt 4 ]; then
		PARTITION_CMD+="n\np\n$DEVNUM\n\n+$size\n"
	elif [ $DEVNUM -eq 5 ]; then
		PARTITION_CMD+="n\ne\n\n\nn\n\n+$size\n"
	elif [ $size = "all" ]; then
		PARTITION_CMD+="n\n\n\n"
	else
		PARTITION_CMD+="n\n\n+$size\n"
	fi
done

dump_partition_table
# echo -e $PARTITION_CMD
echo $DEV_PATH
echo $FORCE_FORMAT

PARTITION_CMD+='w'
echo -e $PARTITION_CMD > $PARTITION_TABLE
# error "partition table debugs"

# step 3: if DEV_PATH exits, continue
if ! [ -b $DEV_PATH ]; then
	error "$DEV_PATH do not exits! check your config file!"
fi

# step 4:(TODO) if partition table num do not fit, and boot.bin exit
# or FORCE_FORMAT is true, and boot.bin exit
# do partition job
# NOTICE: if partition table do not fit and boot.bin do not exit, just quit
#fdisk -l $DEV_PATH > $PARTITION_LIST_F
#for line in `cat $PARTITION_LIST_F | tr -s ' ' | sed -e 's/ /@/g'`
#do
#	line=`echo $line | sed -e 's/@/ /g'`
#	if [ $FORCE_FORMAT = 'y' ]; then
#		break
#	fi
#
#	#line=`echo $line | tr -s ' '`
#	echo $line | grep '^/dev/' > /dev/null 2>&1
#	if [ $? = '0' ]
#	then
#		devpath=`echo $line | sed 's/ .*$//g'`
#		if [ $devpath = ${DEV_PATH}p4 ]; then
#			continue
#		fi
#
#		#echo "value format to y"
#		FORCE_FORMAT='y'
#		i=0
#		while [ "$i" -lt "$DEVNUM" ]
#		do
#			i=$((i+1))
#			if [ $i -eq 4 ]; then
#				continue
#			fi
#			eval "tmpdev=\${RECORD_DEVPATH_${i}}"
#			if [ $tmpdev = $devpath ]; then
#				#echo "set value to n"
#				FORCE_FORMAT='n'
#				break
#			fi
#		done
#	fi
#done

# echo $FORCE_FORMAT
# error "dump test"

# do partition table
if [ $FORCE_FORMAT = 'y' ] && [ -f "$INSTALL_DIR/boot.bin" ]; then
	#error "the boot.bin file do not exit!"
	#del patish first!
	DEL_PARTITION_CMD="d\n1\nd\n2\nd\n3\nd\n4\n"
	#echo -e $DEL_PARTITION_CMD | fdisk $DEV_PATH > /dev/null 2>&1 || error "fdisk del error"
	# do partition
	#echo -e $PARTITION_CMD | fdisk $DEV_PATH > /dev/null 2>&1 || error "fdisk create error"
	CMD=${DEL_PARTITION_CMD}${PARTITION_CMD}
	echo -e $CMD | fdisk $DEV_PATH > /dev/null 2>&1 || error "fdisk create error"
fi

# do fomatting vfat job
i=0
while [ "$i" -lt "$DEVNUM" ]
do
	i=$((i+1))
	if [ $i -eq 4 ]; then
		continue
	fi

	eval "FILESYSTEM=\${RECORD_FILESYSTEM_${i}}"
	eval "DEVPATH=\${RECORD_DEVPATH_${i}}"
	if [ $FILESYSTEM = 'vfat' ] && [ -b $DEVPATH ]; then
		mkdosfs $DEVPATH
	fi
done

#error "test partition table"

#step 5: dd file
i=0
while [ "$i" -lt "$DEVNUM" ]
do
	i=$((i+1))
	if [ $i -eq 4 ]; then
		continue
	fi

	eval "FILESYSTEM=\${RECORD_FILESYSTEM_${i}}"
	eval "FILELIST=\${RECORD_FILELIST_${i}}"
	eval "OUTPUT_DEV=\${RECORD_DEVPATH_${i}}"
	eval "CMD=\${RECORD_CMD_${i}}"
	for file in `echo $FILELIST | sed -e 's/,/ /g'`;do
		if [ -f "$INSTALL_DIR/$file" ] && [ -b $OUTPUT_DEV ]; then
			if [ $CMD = 'cp' ]; then
				#mount and cp
				mkdir /tmp/emmc
				mount -t $FILESYSTEM $OUTPUT_DEV /tmp/emmc
				cp -r $INSTALL_DIR/$file /tmp/emmc
				sync;sync;sync
				cd /
				umount /tmp/emmc
			elif [ $CMD = 'dd' ]; then
				dd if="$INSTALL_DIR/$file" of=${OUTPUT_DEV}
			fi
		fi
	done
done
