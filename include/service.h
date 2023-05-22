#pragma once

#include "config.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define SV_SERIAL_LEN       16
#define SV_SERIAL_RUNIT_LEN 20
#define SV_HAS_LOGSERVICE   ((void*) 1)

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
	S_EXIT    = 'x',    // kill the fsvs instance
	S_STATUS  = 'a',    // get status of all services
	S_SWITCH  = 'l',    // change runlevel
	S_ENABLE  = 'E',    // of extra disable
	S_DISABLE = 'D',    // of extra disable
} sv_command_t;

typedef enum sv_command_runit {
	R_DOWN  = 'd',
	R_UP    = 'u',
	R_EXIT  = 'x',
	R_TERM  = 't',
	R_KILL  = 'k',
	R_PAUSE = 'p',
	R_CONT  = 'c',
	R_ONCE  = 'o',
	R_ALARM = 'a',
	R_HUP   = 'h',
	R_INT   = 'i',
	R_QUIT  = 'q',
	R_USR1  = '1',
	R_USR2  = '2',
} sv_command_runit_t;

typedef enum service_state {
	STATE_INACTIVE,
	STATE_SETUP,
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
	S_DOWN,
	S_FORCE_DOWN,    // force down (manual)
	S_ONCE,
	S_RESTART,
} service_restart_t;

typedef struct service {
	char              name[SV_NAME_MAX];    // name of service
	int               dir;                  // dirfd
	service_state_t   state;
	int               control;           // fd to supervise/control
	pid_t             pid;               // pid of run
	time_t            status_change;     // last status change
	pipe_t            log_pipe;          // pipe for logging
	service_restart_t restart_manual;    // should restart on exit
	service_restart_t restart_file;      // should restart on exit
	bool              restart_final;     // combined restart + dependency (only set client-side)
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


extern const char* command_error[];
extern const char* command_string[];

extern service_t    services[];
extern int          services_size;
extern char         runlevel[];
extern int          service_dir;
extern int          null_fd;
extern int          control_socket;
extern bool         daemon_running;
extern bool         verbose;
extern dependency_t depends[];
extern int          depends_size;
extern const char*  service_dir_path;


char        service_get_command(const char* command);
int         service_command(char command, char extra, const char* service, service_t* response, int response_max);
int         service_handle_command(void* s, sv_command_t command, uint8_t extra, service_t** response);
int         service_pattern(const char* name, service_t** dest, int dest_max);
int         service_refresh(void);
int         service_supervise(const char* service_dir, const char* runlevel, bool force_socket);
service_t*  service_get(const char* name);
service_t*  service_register(int dir, const char* name, bool is_log_service);
void        service_check_state(service_t* s, bool signaled, int return_code);
void        service_handle_socket(int client);
void        service_load(service_t* s, const uint8_t* buffer);    // for fsvc
void        service_send(service_t* s, int signal);
void        service_start(service_t* s, bool* changed);
void        service_stop(service_t* s, bool* changed);
void        service_store(service_t* s, uint8_t* buffer);    // for fsvs
void        service_store_runit(service_t* s, uint8_t* buffer);
const char* service_store_human(service_t* s);
void        service_update_dependency(service_t* s);
bool        service_need_restart(service_t* s);
void        service_run(service_t* s);
void        service_init_status(service_t* s);
void        service_update_status(service_t* s);
void        service_handle_command_runit(service_t* s, sv_command_runit_t command);
void        service_handle_stage(int stage);
