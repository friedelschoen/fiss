# halt 8 "MAY 2023" "0.2.2" "fiss man page"

## NAME

halt, reboot, poweroff - stop the system

## SYNOPSIS

`poweroff` \[`-n`] \[`-f`] \[`-d`] \[`-w`] \[`-b`]

`reboot` \[`-n`] \[`-f`] \[`-d`] \[`-w`] \[`-b`]

`halt` \[`-n`] \[`-f`] \[`-d`] \[`-w`] \[`-b`]

## DESCRIPTION

`poweroff` / `reboot` / `halt` tells `init` to stop running services and stop the system. Invoked without `-f`, it is a shortcut for `init <0|6>`

## OPTIONS

`-n`
Don't sync devices, this doensn't imply that the kernel is not synced already

`-f`
Forces halt / reboot without notifying `init`. Forcing the system to halt should be the last hope and can be dangerous, you should not try it

`-d`
Don't write the _wtmp_ record

`w`
Only write the wtmp record, no further action

## SEE ALSO

finit(1), fsvc(8), fsvs(8), halt(8), modules-load(8), shutdown(8),

## AUTHOR

Based on the version of Leah Neukirchen \<leah@vuxu.org\>, rewritten by Friedel Schön \<derfriedmundschoen@gmail.com\>
