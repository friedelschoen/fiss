# modules-load 8 "MAY 2023" "%VERSION%" "fiss man page"

## NAME

`modules-load` - configure kernel modules at boot

## SYNOPSIS

`modules-load` \[`-nv`]

## DESCRIPTION

modules-load reads files which contain kernel modules to load during boot from the list of locations below.

`-n`
dry-run mode. This option does everything but actually insert or
delete the modules.

`-v`
verbose mode. Print messages about what the program is doing.

## FILES

Configuration files are read from the following locations:

_/etc/modules-load.d/\*.conf_

_/run/modules-load.d/\*.conf_

_/usr/lib/modules-load.d/\*.conf_

The configuration files should simply contain a list of kernel module
names to load, separated by newlines. Empty lines and lines whose first
non-whitespace character is # or ; are ignored.

## HISTORY

This program is a replacement for the modules-load utility provided by
systemd.

## AUTHOR

Leah Neukirchen, leah@vuxu.org.
