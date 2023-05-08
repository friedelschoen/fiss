# fsvc 1 "MAY 2023" "01.0" "fiss man page"

## NAME

fsvc - fiss' service controller

## SYNOPSIS

`fsvc` `start` \[_options_] \[`--pin_`] \<service>

`fsvc` `stop` \[_options_] \[`--pin`] \<service>

`fsvc` `enable` \[_options_] \[`--once`] \<service>

`fsvc` `disable` \[_options_] \[`--once`] \<service>

`fsvc` `kill` \[_options_] \<service> \<signal|signo>

`fsvc` `status` \[_options_] \[`--check`] \[\<service>]

`fsvc` `pause` \[_options_] \<service>

`fsvc` `resume` \[_options_] \<service>

`fsvc` `switch` \[_options_] \[`--reset`] \<runlevel>

## DESCRIPTION

`fsvc` is a command line tool for controlling services on a FISS system. It provides various commands for starting, stopping, enabling, disabling, sending signals to, checking the status of, pausing, resuming, and switching the runlevel of services.

## OPTIONS

The following options are available for every command:

`-q, --short`
Will print brief information about the service.

`-r, --runlevel <runlevel>`
Will use \fBrunlevel\fB instead of _default_.

`-s, --service-dir <path>`
Will use `path` as service directory instead of _/etc/fiss/service.d_.

`-v, --verbose`
Prints verbose information.

`-V, --version`
Prints the version and exits.

The following options are available per command:

`-p, --pin`
Pins the state. If issued with `start` it will cause the service to restart.
If issued with `stop` it will cause the service to be hold down.

`-o, --once`
Specifies that the service should only be enabled/disabled once and not automatically started/stopped on subsequent bootups.

`-c, --check`
Command will return _0_ if the specified service is active otherwise _1_

`-r, --reset`
Specifies that the switch command should reset all running services (if manually set to up/down)

## COMMANDS

The following commands are available:

`start`
Starts the specified service. A synonym is `up`.

`stop`
Stops the specified service. A synonym is `down`.

`enable`
Enables the specified service, causing it to automatically start on subsequent bootups.

`disable`
Disables the specified service, causing it to not start automatically on subsequent bootups.

`kill`
Sends the specified signal or signal number to the specified service. A synonym is `send`.

`status`
Displays the status of the specified service. If no service is specified, displays the status of all services.

`pause`
Pauses the specified service.

`resume`
Resumes the specified service.

`switch`
Switches the runlevel to the specified value.

## SEE ALSO

finit(1), fsvc(8), fsvs(8), halt(8), modules-load(8), shutdown(8),

## AUTHOR

Friedel Schön \<derfriedmundschoen@gmail.com>
