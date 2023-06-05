#pragma once

#include "config.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>


enum service_command {
	X_UP    = 'u',    // starts the services, pin as started
	X_DOWN  = 'd',    // stops the service, pin as stopped
	X_XUP   = 'U',    // starts the service, set restart (d = down, f = force_down, o = once, u = up)
	X_XDOWN = 'D',    // stops the service, set restart (d = down, f = force_down, o = once, u = up)
	X_ONCE  = 'o',    // starts the service, pin as once
	X_TERM  = 't',    // same as down
	X_KILL  = 'k',    // sends kill, pin as stopped
	X_PAUSE = 'p',    // pauses the service
	X_CONT  = 'c',    // resumes the service
	X_RESET = 'r',    // resets the service
	X_ALARM = 'a',    // sends alarm
	X_HUP   = 'h',    // sends hup
	X_INT   = 'i',    // sends interrupt
	X_QUIT  = 'q',    // sends quit
	X_USR1  = '1',    // sends usr1
	X_USR2  = '2',    // sends usr2
	X_EXIT  = 'x',    // does nothing
};

enum service_state {
	STATE_INACTIVE,
	STATE_SETUP,
	STATE_STARTING,
	STATE_ACTIVE_FOREGROUND,
	STATE_ACTIVE_BACKGROUND,
	STATE_ACTIVE_DUMMY,
	STATE_STOPPING,
	STATE_FINISHING,
	STATE_DEAD,
};

enum service_exit {
	EXIT_NONE,
	EXIT_NORMAL,
	EXIT_SIGNALED,
};

enum service_restart {
	S_DOWN,
	S_FORCE_DOWN,    // force down (manual)
	S_ONCE,
	S_RESTART,
};

struct service_serial {
	uint8_t status_change[8];
	uint8_t state;
	uint8_t return_code;
	uint8_t fail_count;
	uint8_t flags;
	uint8_t pid[4];
	uint8_t paused;
	uint8_t restart;
	uint8_t force_down;
	uint8_t state_runit;
};

typedef struct service {
	char            name[SV_NAME_MAX];    // name of service
	int             dir;                  // dirfd
	int             state;
	int             control;           // fd to supervise/control
	pid_t           pid;               // pid of run
	time_t          status_change;     // last status change
	pipe_t          log_pipe;          // pipe for logging
	int             restart_manual;    // should restart on exit
	int             restart_file;      // should restart on exit
	bool            restart_final;     // combined restart + dependency (only set client-side)
	int             last_exit;         // stopped signaled or exited
	int             return_code;       // return code or signal
	uint8_t         fail_count;        // current fail cound
	bool            is_log_service;    // is a log service
	bool            paused;            // is paused
	struct service* log_service;       // has a log_server otherwise NULL
	bool            is_dependency;     // only set by service_load
} service_t;

extern service_t   services[];
extern int         services_size;
extern char        runlevel[];
extern int         service_dir;
extern int         null_fd;
extern bool        daemon_running;
extern service_t*  depends[][2];
extern int         depends_size;
extern const char* service_dir_path;


void        service_decode(service_t* s, const void* buffer);    // for fsvc
void        service_encode(service_t* s, void* buffer);          // for fsvs
service_t*  service_get(const char* name);
void        service_handle_client(int client);
void        service_handle_command(service_t* s, char command, char data);
void        service_handle_exit(service_t* s, bool signaled, int return_code);
void        service_kill(service_t* s, int signal);
bool        service_need_restart(service_t* s);
int         service_pattern(const char* name, service_t** dest, int dest_max);
int         service_refresh_directory(void);
service_t*  service_register(int dir, const char* name, bool is_log_service);
void        service_run(service_t* s);
int         service_send_command(char command, char extra, const char* service, service_t* response, int response_max);
void        service_stage(int stage);
void        service_start(service_t* s);
const char* service_status_name(service_t* s);
void        service_stop(service_t* s);
int         service_supervise(const char* service_dir, const char* runlevel);
void        service_update_dependency(service_t* s);
bool        service_is_dependency(service_t* s);
void        service_update_state(service_t* s, int state);
