# zzz 8 "MAY 2023" "0.1.2" "fiss man page"

## NAME

zzz - suspend or hibernate your system

## SYNOPSIS

`zzz [-nSzZRH]`

## DESCRIPTION

`zzz` is a simple utility to hibernate or suspend your computer and part of the fiss-system. It supports suspend/resume-hooks.

`-n, --noop`
dry-run, sleep for 5sec instead of actually running ACPI actions.

`-S, --freeze`
enter low-power idle mode

`-z, --suspend`
suspend to RAM, this is the default behaviour of `zzz`

`-Z, --hibernate`
hibernate to disk and power off

`-R, --reboot`
hibernate to disk and reboot (useful for switching operating systems)

`-H, --hybrid`
hibernate to disk and suspend

## HOOKS

Before suspending, `zzz` executes _/usr/share/fiss/suspend_ which intents to execute scripts inside _/etc/zzz.d/suspend_ in alphanumeric order. After resuming, `zzz` executes _/usr/share/fiss/resume_ which intents to execute scripts inside _/etc/zzz.d/resume_ in alphanumeric order.

## SEE ALSO

fiss-init(8), runsvdir(8), runsvchdir(8), sv(8), runsv(8), chpst(8), utmpset(8), svlogd(8)

## AUTHOR

Based on the version of Leah Neukirchen \<leah@vuxu.org\>, rewritten by Friedel Schön \<derfriedmundschoen@gmail.com\>
