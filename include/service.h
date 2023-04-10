#pragma once

#include "config.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#define SV_SERIAL_LEN     17
#define SV_HAS_LOGSERVICE ((void*) 1)

#define EBADCMD 1    // command not found
#define ENOSV   2    // service required
#define EBADSV  3    // no matching services
#define EBEXT   4    // invalid extra

typedef enum {
	S_START   = 'u',    // start if not running and restart if failed
	S_STOP    = 'd',    // stop if running and not restart if failed
	S_SEND    = 'k',    // + signal | send signal to service
	S_PAUSE   = 'p',    // pause service (send SIGSTOP)
	S_RESUME  = 'c',    // unpause service (send SIGCONT)
	S_REVIVE  = 'v',    // revive died service
	S_UPDATE  = 'g',    // force update info // todo
	S_EXIT    = 'x',    // kill the fsvs instance
	S_STATUS  = 'a',    // get status of all services
	S_CHLEVEL = 'l',    // change runlevel
} sv_command_t;

typedef enum service_state {
	STATE_INACTIVE,
	STATE_STARTING,
	STATE_ACTIVE_DUMMY,
	STATE_ACTIVE_FOREGROUND,
	STATE_ACTIVE_BACKGROUND,
	STATE_ACTIVE_PID,
	STATE_FINISHING,
	STATE_STOPPING,
	STATE_DEAD,
} service_state_t;

typedef enum service_exit {
	EXIT_NONE,
	EXIT_NORMAL,
	EXIT_SIGNALED,
} service_exit_t;

typedef enum service_restart {
	S_NONE,
	S_RESTART,
	S_ONCE,
} service_restart_t;

typedef struct service {
	char              name[SV_NAME_MAX];    // name of service
	service_state_t   state;
	pid_t             pid;               // pid of run
	time_t            status_change;     // last status change
	pipe_t            log_pipe;          // pipe for logging
	service_restart_t restart_manual;    // should restart on exit
	service_restart_t restart_file;      // should restart on exit
	service_exit_t    last_exit;         // stopped signaled or exited
	int               return_code;       // return code or signal
	uint8_t           fail_count;        // current fail cound
	bool              is_log_service;    // is a log service
	bool              paused;
	struct service*   log_service;
} service_t;

typedef struct dependency {
	service_t* service;
	service_t* depends;
} dependency_t;


extern string command_error[];
extern string command_string[];

extern service_t    services[];
extern int          services_size;
extern string       runlevel;
extern int          null_fd;
extern int          control_socket;
extern bool         daemon_running;
extern bool         verbose;
extern string       service_dir;
extern dependency_t depends[];
extern int          depends_size;


char       service_get_command(string command);
int        service_command(char command, char extra, string service, service_t* response, int response_max);
int        service_handle_command(service_t* s, sv_command_t command, uint8_t extra, service_t** response);
int        service_pattern(string name, service_t** dest, int dest_max);
int        service_refresh();
int        service_supervise(string service_dir, string runlevel, bool force_socket);
service_t* service_get(string name);
service_t* service_register(string name, bool is_log_service);
void       service_check_state(service_t* s, bool signaled, int return_code);
void       service_handle_socket(int client);
void       service_load(service_t* s, const uint8_t* buffer);    // for fsvc
void       service_send(service_t* s, int signal);
void       service_start(service_t* s, bool* changed);
void       service_stop(service_t* s, bool* changed);
void       service_store(service_t* s, uint8_t* buffer);    // for fsvs
void       service_update_dependency(service_t* s);
bool       service_need_restart(service_t* s);