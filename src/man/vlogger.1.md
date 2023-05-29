# vlogger 1 "MAY 2023" "%VERSION%" "fiss man page"

## NAME

`vlogger` - log messages to syslog or an arbitrary executable

## SYNOPSIS

`vlogger` \[`-isS`] \[`-f` file] \[`-p` pri] \[`-t` tag] \[message ...]

## DESCRIPTION

The vlogger utility writes messages to the system log or an arbitrary executable.

If vlogger is executed as logger it will always use the system log and behave like the regular logger(1).

Without message arguments vlogger reads messages from stdin or the file specified with the -f flag. If the /etc/vlogger executable exists vlogger executes it with tag, level and facility as arguments, replacing
the vlogger process.

If vlogger is executed as a log service for fiss(8) or another daemontools like supervision suite it uses the service name as default tag. As example if vlogger is linked to /var/service/foo/log/run it uses “foo” as tag and “daemon.notice” as pri.

The options are as follows:

`-f file`
Read lines from the specified file. This option cannot be combine message arguments.

`-i`
Log the PID of the vlogger process. Only supported if syslog(3) is used.

`-p pri`
The. pri can be facility.level or just facility. See FACILITIES, LEVELS or syslog(3). The default is “user.notice”.

`-S`
Force vlogger to use syslog(3) even if /etc/vlogger exists.

`-s`
Output the message to standard error, as well as syslog(3). Only supported if syslog(3) is used.

`-t tag`
Defines the openlog(3) ident which is used as prefix for each log message or passed as first argument to /etc/vlogger. The default is the LOGNAME environment variable.

`message`
Write the message to the system log.

## FACILITIES

auth
authpriv
cron
daemon
ftp
kern can not be used from userspace replaced with daemon.
lpr
mail
news
syslog
user
uucp
local[0-7]
security deprecated synonym for auth.

## LEVELS

emerg
alert
crit
err
warning
notice
info
debug
panic deprecated synonym for emerg.
error deprecated synonym for err.
warn deprecated synonym for warning.

## FILES

`/etc/vlogger`
An optional executable file that is used to handle the messages. It is executed with tag, level and facility as arguments and replaces the vlogger process.

## EXIT STATUS

The vlogger utility exits 0 on success, and >0 if an error occurs.

## EXAMPLES

`/etc/vlogger:`
#!/bin/sh
exec svlogd /var/log/$1

## SEE ALSO

logger(1), syslog(3), svlogd(8)

## HISTORY

This program is a replacement for the logger utility provided by util-linux and forked from daemontools.

## AUTHORS

Duncan Overbruck <mail@duncano.de>

## LICENSE

vlogger is in the public domain.

To the extent possible under law, the creator of this work has waived all copyright and related or neighboring rights to this work.

http://creativecommons.org/publicdomain/zero/1.0/
