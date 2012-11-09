#!/bin/bash
#echo $1
#echo $3
# table list of configure file
# devnum filesystem cmd filelist size
devnum=
filesystem=
cmd=
filelist=
size=

INSTALL_DIR=/tmp/sd/
DEV_PATH=
ITEM_NUM=5
ITEM=

export PARTITION_FINA="abc"

function error()
{
	echo $1
	exit -1
}

if ! [ -f "$1" ]; then
	error "the file $1 do not exits!"
fi

config_file=$1

# parse the config file
cat $config_file | while read line
do
	echo $line | grep '^/dev/' > /dev/null 2>&1
	if [ $? = '0' ]; then
		DEV_PATH=$line
		continue
		#echo $DEV_PATH 
		#exit 0
	fi
	
	echo "**********"
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

#		if [ $item_num = '4' ]; then
#			echo $item
#		fi
	done

	echo "$devnum $filesystem $cmd $filelist $size"
	
	#eval "prev_${devnum}_${filesystem}=$filesystem"
	if [ $devnum -lt 4 ]; then
		gen_cmd="n\np\n$devnum\n\n+$size"
		PARTITION_CMD+="n\np\n$devnum\n\n+$size\n"
	elif [ $devnum -eq 5 ]; then
		gen_cmd="n\ne\n\n\nn\n\n+$size"
		PARTITION_CMD+="n\ne\n\n\nn\n\n+$size\n"
	elif [ $size = "all" ]; then
		gen_cmd="n\n\n"
		PARTITION_CMD+="n\n\n\n"
	else
		gen_cmd="n\n\n+$size"
		PARTITION_CMD+="n\n\n+$size\n"
	fi

	export PARTITION_FINA=$PARTITION_CMD
	echo -e $gen_cmd >> tabletest
done

	echo -e "w" >> tabletest

	echo $DEV_PATH
echo "continul"
