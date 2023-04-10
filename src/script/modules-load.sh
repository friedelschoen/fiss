#!/bin/sh
# modules-load [-n] [-v] - modules-load.d(5) compatible kernel module loader

# Set the PATH variable to include /bin and /sbin
export PATH=/bin:/sbin

# Find modules to load based on modules-load parameters and configuration files
find_modules() {
	# Parameters passed as modules-load= or rd.modules-load= in kernel command line.
	sed -nr 's/,/\n/g;s/(.* |^)(rd\.)?modules-load=([^ ]*).*/\3/p' /proc/cmdline
	
	# Find files /{etc,run,usr/lib}/modules-load.d/*.conf in that order.
	find -L /etc/modules-load.d /run/modules-load.d /usr/lib/modules-load.d \
		-maxdepth 1 -name '*.conf' -printf '%p %P\n' 2>/dev/null |
		# Load each basename only once.
		sort -k2 -s | uniq -f1 | cut -d' ' -f1 |
		# Read the files, output all non-empty, non-comment lines.
		tr '\012' '\0' | xargs -0 -r grep -h -v -e '^[#;]' -e '^$'
}

# Load modules using modprobe
load_modules() {
	# Call modprobe on the list of modules
	tr '\012' '\0' | xargs -0 -r modprobe -ab "$@"
}

# Find and load modules
find_modules | load_modules