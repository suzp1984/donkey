#!/bin/bash

function error()
{
	echo $1
	exit -1;
}

PARTITION_CMD="d\n1\nd\n2\nd\n3\nd\n4\nw\n"
echo "start partition"

echo -e $PARTITION_CMD | fdisk /dev/sdb > /dev/null 2>&1 || error "fdisk error"
