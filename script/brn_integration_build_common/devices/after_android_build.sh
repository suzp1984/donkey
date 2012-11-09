#!/bin/bash

BACKUP_DIR="/backup/branch/`$PYTHON_PARSER -g $DEVICE_NAME branch`"
OS_WALK_DIR="$ANDROID_BUILD_TOP/$BACKUP_DIR"

function recovery_branch()
{
	if [ "x$1" = "x" ]; then
		exit -1
	fi

	backup_file=$1
	des=`echo $backup_file | sed -e 's#'"$BACKUP_DIR"'##'`
	mv $backup_file $des
	datasafe_tags set $des
}

# do recovery
for file in `$PYTHON_DIR_WALK $OS_WALK_DIR`; do
	#echo $file
	recovery_branch $file
done
