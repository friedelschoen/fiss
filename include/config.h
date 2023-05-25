#pragma once

// environment variable where the current runlevel is stored
#ifndef SV_RUNLEVEL_DEFAULT_ENV
#	define SV_RUNLEVEL_DEFAULT_ENV "SERVICE_RUNLEVEL"
#endif

// seconds to wait for a service before it gets killed
#ifndef SV_STOP_TIMEOUT
#	define SV_STOP_TIMEOUT 5
#endif

// maximal characters a service-dir can have
#ifndef SV_NAME_MAX
#	define SV_NAME_MAX 512
#endif

// maximal dependencies a service can have
#ifndef SV_DEPENDS_MAX
#	define SV_DEPENDS_MAX 16
#endif

// maximal amount a service may fail
#ifndef SV_FAIL_MAX
#	define SV_FAIL_MAX 32
#endif

// maximal amount of services that can be registered
#ifndef SV_SERVICE_MAX
#	define SV_SERVICE_MAX 128
#endif

// default runlevel
#ifndef SV_RUNLEVEL_DEFAULT
#	define SV_RUNLEVEL_DEFAULT "default"
#endif

// path to service-dir
#ifndef SV_SERVICE_DIR
#	define SV_SERVICE_DIR "/etc/service.d"
#endif

// path to start-script
#ifndef SV_START_EXEC
#	define SV_START_EXEC "/usr/share/fiss/start"
#endif

// path to stop-script
#ifndef SV_STOP_EXEC
#	define SV_STOP_EXEC "/usr/share/fiss/stop"
#endif

// path to suspend-script
#ifndef SV_SUSPEND_EXEC
#	define SV_SUSPEND_EXEC "/usr/share/fiss/suspend"
#endif

// path to resume-script
#ifndef SV_RESUME_EXEC
#	define SV_RESUME_EXEC "/usr/share/fiss/resume"
#endif

// the current version
#ifndef SV_VERSION
#	define SV_VERSION "0.2.2"
#endif

// time to wait to accept new connection
#ifndef SV_ACCEPT_INTERVAL
#	define SV_ACCEPT_INTERVAL 1    // seconds
#endif

// control socket (%s is the runlevel)
#ifndef SV_CONTROL_SOCKET
#	define SV_CONTROL_SOCKET "/run/fiss/control-%s.socket"
#endif

// maximal size of <service>/params
#ifndef SV_PARAM_FILE_MAX
#	define SV_PARAM_FILE_MAX 16384    // 16kb
#endif

// maximal size of a param in ./params
#ifndef SV_PARAM_FILE_LINE_MAX
#	define SV_PARAM_FILE_LINE_MAX 1024    // 16kb
#endif

// maximal size of a line in ./env
#ifndef SV_ENV_FILE_LINE_MAX
#	define SV_ENV_FILE_LINE_MAX 1024    // 16kb
#endif

// shell to enter if fiss failed
#ifndef SV_RESCUE_SHELL
#	define SV_RESCUE_SHELL "/bin/bash"
#endif

// max dependencies in the dep-tree
#ifndef SV_DEPENDENCY_MAX
#	define SV_DEPENDENCY_MAX 512
#endif

// max arguments a service can have
#ifndef SV_ARGUMENTS_MAX
#	define SV_ARGUMENTS_MAX 16
#endif

// maximal count of environment variables in ./env
#ifndef SV_ENV_MAX
#	define SV_ENV_MAX 16
#endif

// defines the directory where logs are stored
#ifndef SV_LOG_DIR
#	define SV_LOG_DIR "/var/log/fiss"
#endif

#ifndef SV_PID_BUFFER
#	define SV_PID_BUFFER 16
#endif

#ifndef SV_SOCKET_SERVICE_MAX
#	define SV_SOCKET_SERVICE_MAX 128
#endif

#ifndef SV_USER_BUFFER
#	define SV_USER_BUFFER 256
#endif

#ifndef SV_USER_GROUP_MAX
#	define SV_USER_GROUP_MAX 32
#endif

#ifndef SV_VLOGGER_BUFFER
#	define SV_VLOGGER_BUFFER 1024
#endif

#ifndef SV_RUNIT_COMPAT
#	define SV_RUNIT_COMPAT 1
#endif

#ifndef SV_STATUS_WAIT
#	define SV_STATUS_WAIT 5
#endif
