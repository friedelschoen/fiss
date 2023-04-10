#!/bin/sh

fsvc=/sbin/fsvc
fsvc_lsb=fsvc-lsb

name=$(basename $0)

if [ "$name" = "$fsvc_lsb" ]; then
	echo "warning: calling fsvs-lsb without service"
	echo "  probabally this will cause an error but maybe it's intentionally"
fi

if [ -z "$1" ]; then 
	echo "error: missing <command>"
	echo "usage: $0 <command>"
	exit 1
fi

exec $fsvc $name $1 