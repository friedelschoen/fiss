#pragma once

// environment variable where the current runlevel is stored
#define SV_RUNLEVEL_ENV "SERVICE_RUNLEVEL"
// seconds to wait for a service before it gets killed
#define SV_STOP_TIMEOUT 5
// maximal characters a service-dir can have
#define SV_NAME_MAX 512
// maximal dependencies a service can have
#define SV_DEPENDS_MAX 16
// maximal amount a service may fail
#define SV_FAIL_MAX 32
// maximal amount of services that can be registered
#define SV_SERVICE_MAX 128
// default runlevel
#define SV_RUNLEVEL "default"
// path to service-dir
#define SV_SERVICE_DIR "/etc/service.d"
// path to start-script
#define SV_START_EXEC "/usr/share/fiss/start"
// path to stop-script
#define SV_STOP_EXEC "/usr/share/fiss/stop"
// the current version
#define SV_VERSION "0.1.0"
// time to wait to accept new connection
#define SV_ACCEPT_INTERVAL 1    // seconds
// control socket (%s is the runlevel)
#define SV_CONTROL_SOCKET "/run/fiss/control-%s.socket"
// maximal size of <service>/params
#define SV_PARAM_FILE_MAX 16384    // 16kb

#define SV_PARAM_FILE_LINE_MAX 1024    // 16kb

#define SV_ENV_FILE_LINE_MAX 1024    // 16kb
// shell to enter if fiss failed
#define SV_RESCUE_SHELL "/bin/bash"
// max dependencies in the dep-tree
#define SV_DEPENDENCY_MAX 512
// max arguments a service can have
#define SV_ARGUMENTS_MAX 16
#define SV_ENV_MAX       16

#define SV_LOG_DIR "/var/log/fiss"