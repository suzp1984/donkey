#!/bin/bash
BRANCH_DIR=/branch/`$PYTHON_PARSER -g $DEVICE_NAME branch`
OS_WALK_DIR="$ANDROID_BUILD_TOP/$BRANCH_DIR"

function replace_backup_branch()
{
	if [ "x$1" = "x" ]; then
		exit -1
	fi

	src=$1
	des=`echo $src | sed -e 's#'"$BRANCH_DIR"'##'`
	backup_file=`echo $src | sed -e 's#'"$BRANCH_DIR"'#/backup'"$BRANCH_DIR"'#'`
	# do backup
	mkdir -p `dirname $backup_file`
	cp $des $backup_file

	#replace
	cp $src $des
	datasafe_tags set $des
}


for file in `$PYTHON_DIR_WALK $OS_WALK_DIR`; do
	replace_backup_branch $file
done
