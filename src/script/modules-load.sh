#!/bin/sh
# modules-load [-n] [-v] - modules-load.d(5) compatible kernel module loader

# Set the PATH variable to include /bin and /sbin
export PATH=/bin:/sbin

(
	sed -nr 's/,/\n/g;s/(.* |^)(rd\.)?modules-load=([^ ]*).*/\3/p' /proc/cmdline
	
	grep -hve '^[#;]' -e '^$' /etc/modules-load.d/* /run/modules-load.d/* /usr/lib/modules-load.d/* 2> /dev/null
) | uniq | xargs -r modprobe -ab $@