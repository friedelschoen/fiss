FRIEDEL'S INITIALIZATION and SERVICE SUPERVISION
================================================================================

--- Components -----------------------------------------------------------------

/sbin/finit - initialisation
- the init-executable, it handles the startup and initialisation.
  1. execute /etc/fiss/start
  2. execute /bin/fsvs at /etc/fiss/services until stopped
  3. execute /etc/fiss/stop
  4. halt

/sbin/fsvs - service superviser

/sbin/fsvc - service controller


--- Configuration --------------------------------------------------------------

in enum.h are some definitions (#define ...) to configure built-in variables:

SV_SERVICE_DIR_ENV (default: "SERVICE_DIR")
- environment variable where the current service-dir is stored:

SV_RUNLEVEL_ENV (default: "SERVICE_RUNLEVEL")
- environment variable where the current runlevel is stored

SV_STOP_TIMEOUT (default: 5)
- seconds to wait for a service before it gets killed

SV_NAME_MAX (default: 1024)
- maximal characters a service-dir can have

SV_DEPENDS_MAX (default: 16)
- maximal dependencies a service can have

SV_FAIL_MAX (default: 127)  
- maximal amount a service may fail (max. 255 as it's stored in uint8)

SV_SERVICE_MAX (default: 128)
- maximal amount of services that can be registered

SV_SUPERVISE_DIR (default: undefined)
- path to supervision directory (undefined if supervision in service-dir)

SV_DAEMONTOOLS (default: undefined)
- define to enable daemontools' supervice-dir 

--- Installation ---------------------------------------------------------------

$ make
# make install


--- Service Directory ----------------------------------------------------------

every service needs to be a directory in
  {/etc/fiss/service -> FISS_PATH}, with either or both: run and depends

run - the service, if it dies it will be restarted

depends - services which needs to be running before this
  service will be started, one service per line

finish - if run dies, this script will be called,
  the return-code will be ignored, 'run will be restarted after
  after 'finish exits

zombie - service cannot die, also if failed more than FAIL_LIMIT
  service' fail_count may overflow and reset to zero

params - params to run

autostart-{...} - start automatically when init is running
autostart-{...}-once - start automatically when init is running but once

unique - doesn't spawn if it's running already by an other session

socket - path to socket-file where stdin and stdout/stderr will be redirected
  (cannot exist with log/)

log/ - logging service

a service is considered started, if it's running {5sec -> FISS_FAIL_TIMEOUT}
if it dies FISS_FAIL_MAX times with an exit code or signal it will be considered as dead

/run/services/{runlevel}-{service}/:

control - fifo to control service


--- Supervice Directory --------------------------------------------------------

control: a fifo to control the service (see control-commands)
info:    a file containing the current status (machine readable - fiss-format)
lock:    an empty file, if existing it's blocking spawning (useful if running fiss-sv multiple times)

additional files if SV_DAEMONTOOLS is enabled:
ok:      an unused fifo to indicate if fiss is still running (as a fifo always needs two ends)
pid:     a file containing the pid of the running service (human readable)
stat:    a file containing the current status (human readable)
status:  a file containing the current status (machine readable - daemontools-format)


--- Commands -------------------------------------------------------------------

c   -> sig cont (and unset pause-flag)
d   -> down (stop service if running and unset restart-flag)
g   -> force update info
o   -> once (start service if not running and unset restart-flag)
p   -> sig pause (and set pause-flag)
r   -> restart (stop service if running and start)
s % -> send signal (requires extra byte $signal)
u   -> up (start service if not running and set restart-flag)
v   -> revive (start service if not running and set zombie-flag)
w   -> start without dependencies

additional commands if SV_DAEMONTOOLS is enabled:
a -> sig alarm
h -> sig hup
i -> sig int
k -> sig kill
q -> sig quit
t -> sig term
x -> exit (no operation)
1 -> sig usr1
2 -> sig usr2