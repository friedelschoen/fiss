# shutdown 8 "MAY 2023" "0.2.2" "fiss man page"

## NAME

`shutdown` – bring down the system

## SYNOPSIS

`shutdown` \[`-rhP`] \[`-fF`] \[now | +mins] \[message ...]

## DESCRIPTION

shutdown brings the system down in a secure way. All logged-in users are notified that the system is going down, and login(1) is blocked.

By default, shutdown puts the system into single user mode. Rebooting and halting the system can be done using the following options:

`-c`
Cancel an ongoing shutdown.

`-f`
Enable fast booting; skip fsck(8) on next boot.

`-F`
Force run of fsck(8) on next boot.

`-h`
Halt the system.

`-k`
Don't really shutdown; only send the warning messages to everybody.

`-P`
Poweroff the system.

`-r`
Reboot the system.

`now`
Shutdown without further waiting.

`+mins`
Wait mins minutes before shutting down.

`message`

Message displayed to all users, defaults to "system is going down".

## UNSUPPORTED OPTIONS

This version of shutdown is based on runit(8), the following features are not supported:

`-t secs`
to wait secs seconds between SIGKILL and SIGTERM on shutdown is silently ignored.

`-a`
Use /etc/shutdown.allow.

`-H`
Drop into boot monitor.

`-n`
Don't call init(8).

`hh:mm`
Absolute time specification is not implemented.

## SEE ALSO

fsck(8), halt(8), init(8), poweroff(8), reboot(8), fiss(8), runsvchdir(8)

## AUTHOR

Leah Neukirchen, leah@vuxu.org.