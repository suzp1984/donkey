#!/bin/bash
# version: v3 
# release note: 
#echo $1
#echo $3
# table list of configure file
# devnum filesystem cmd filelist size
devnum=
filesystem=
cmd=
filelist=
size=

# define some constant vars
INSTALL_DIR=/tmp/sd
PARTITION_TABLE=$INSTALL_DIR/partition_cmd
CONFIG_FILE='./cfg.ini'
FORCE_FORMAT='n'
DEV_PATH=

function error()
{
	echo $1
	exit -1
}

function dump_partition_table()
{
	i=0
	while [ "$i" -lt "$devnum" ]
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
	echo $line | grep '^/dev/' > /dev/null 2>&1
	if [ $? = '0' ]; then
		DEV_PATH=$line
		continue
	fi
	
	echo $line | grep '^FORCE_FORMAT' > /dev/null 2>&1
	if [ $? = '0' ]; then
		FORCE_FORMAT=`echo $line | sed 's/^FORCE_FORMAT=//g'`
		continue
	fi

	item_num=0
	for item in $line 
	do
		item_num=$((item_num+1))
		
		case "$item_num" in
			"1")
				devnum=$item;;
			"2")
				filesystem=$item;;
			"3")
				cmd=$item;;
			"4")
				filelist=$item;;
			"5")
				size=$item;;
		esac

	done

	eval "RECORD_FILESYSTEM_${devnum}=$filesystem"
	eval "RECORD_CMD_${devnum}=$cmd"
	eval "RECORD_FILELIST_${devnum}=$filelist"
	eval "RECORD_SIZE_${devnum}=$size"
	
	if [ $devnum -lt 4 ]; then
		PARTITION_CMD+="n\np\n$devnum\n\n+$size\n"
	elif [ "$devnum" -eq 5 ]; then
		PARTITION_CMD+="n\ne\n\n\nn\n\n+$size\n"
	elif [ $size = "all" ]; then
		PARTITION_CMD+="n\n\n\n"
	else
		PARTITION_CMD+="n\n\n+$size\n"
	fi
done

dump_partition_table
echo -e $PARTITION_CMD
error "dump test"

PARTITION_CMD+='w'
echo -e $PARTITION_CMD > $PARTITION_TABLE

# step 3: if DEV_PATH exits, continue
if ! [ -b $DEV_PATH ]; then
	error "$DEV_PATH do not exits! check your config file!"
fi

#step 4:(TODO) if partition table num do not fit, and boot.bin exit
# or FORCE_FORMAT is true, and boot.bin exit
# do partition job
# NOTICE: if partition table do not fit and boot.bin do not exit, just quit
if [ $FORCE_FORMAT = 'y' ] && [ -f "$INSTALL_DIR/boot.bin" ]; then
	#error "the boot.bin file do not exit!"
	#del patish first!
	sh del_partition.sh
	# do partition
	echo -e $PARTITION_CMD | fdisk /dev/sdb > /dev/null 2>&1 || error "fdisk error"
fi

#step 5: dd file
i=0
while [ "$i" -lt "$devnum" ]
do
	i=$((i+1))
	FILELIST=`eval "echo \${RECORD_FILELIST_${i}}"`
	for file in `echo $FILELIST | sed -e 's/,/ /g'`;do
		dd if=$file of=${DEV_PATH}p0$i
	done
done
