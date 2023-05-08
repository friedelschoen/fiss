# finit 1 "MAY 2023" "01.0" "fiss man page"

## NAME

fiss - a UNIX process no 1

## SYNOPSIS

`fsvc`
when running as PID 0 (as init)

`fsvc` \<0|6\>
when running regulary to controll init

## CONTROLLING

If `finit` is invoked by any other user than _root_, it failes.

`finit` 0
halts the system

`finit` 6
reboots the system

## DESCRIPTION

`fiss` must be run as Unix process no 1 if invoked without arguments and handles the boot process in user-land.

This happens in three stages:

## STAGE 1

`fiss` runs _/etc/fiss/start_ and waits for it to terminate. The system's one time tasks are done here. _/etc/fiss/start_ has full control of _/dev/console_ to be able to start an emergency shell if the one time initialization tasks fail. If _/etc/fiss/start_ crashes, `finit` will skip stage 2 and enter stage 3.

## STAGE 2

`fiss` starts all services in _/etc/fiss/service.d_ which should not return until system shutdown; if it crashes, it will be restarted.

## STAGE 3

If `fiss` is told to shutdown the system, it terminates stage 2 if it is running, and runs _/etc/fiss/stop_. The systems tasks to shutdown and possibly halt or reboot the system are done here. If stage 3 returns, `finit` checks if the file

## SIGNALS

`finit` only accepts signals in stage 2.

If `finit` receives a CONT signal and the file `finit`
is told to shutdown the system.

if `finit` receives an INT signal, `finit` restarts the system.

## SEE ALSO

fiss-init(8), runsvdir(8), runsvchdir(8), sv(8), runsv(8), chpst(8), utmpset(8), svlogd(8)

## AUTHOR

Friedel Schön \<derfriedmundschoen@gmail.com\>
