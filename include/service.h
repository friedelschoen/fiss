#pragma once

#include "config.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>


enum service_command {
	X_UP    = 'u',    // starts the services, pin as started
	X_DOWN  = 'd',    // stops the service, pin as stopped
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
	STATE_INACTIVE,             // not started
	STATE_SETUP,                // ./setup running
	STATE_STARTING,             // ./start running
	STATE_ACTIVE_FOREGROUND,    // ./run running
	STATE_ACTIVE_BACKGROUND,    // ./start finished, ./stop not called yet
	STATE_ACTIVE_DUMMY,         // dependencies started
	STATE_STOPPING,             // ./stop running
	STATE_FINISHING,            // ./finish running
	STATE_ERROR,                // something went wrong
};

enum service_exit {
	EXIT_NONE,        // never exited
	EXIT_NORMAL,      // exited
	EXIT_SIGNALED,    // crashed
};

enum service_restart {
	S_DOWN,       // service should not be started
	S_ONCE,       // service should
	S_RESTART,    // service should be started
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

typedef struct service service_t;

struct service {
	char       name[SV_NAME_MAX];    // name of service
	int        state;                // current state
	pid_t      pid;                  // pid of run
	int        dir;                  // dirfd
	int        control;              // fd to supervise/control
	time_t     state_change;         // last status change
	int        restart;              // should restart on exit
	int        last_exit;            // stopped signaled or exited
	int        return_code;          // return code or signal
	uint8_t    fail_count;           // current fail cound
	bool       is_log_service;       // is a log service
	bool       paused;               // is paused
	time_t     stop_timeout;         // stop start-time
	pipe_t     log_pipe;             // pipe for logging
	service_t* log_service;          // has a log_server otherwise NULL
};

extern service_t   services[];
extern int         services_size;
extern int         service_dir;
extern int         null_fd;
extern bool        daemon_running;
extern service_t*  depends[][2];
extern int         depends_size;
extern const char* service_dir_path;


void        service_encode(service_t* s, void* buffer);
service_t*  service_get(const char* name);
void        service_handle_command(service_t* s, char command);
void        service_handle_exit(service_t* s, bool signaled, int return_code);
void        service_kill(service_t* s, int signal);
bool        service_need_restart(service_t* s);
int         service_refresh_directory(void);
service_t*  service_register(int dir, const char* name, bool is_log_service);
void        service_run(service_t* s);
int         service_send_command(char command, char extra, const char* service, service_t* response, int response_max);
void        service_start(service_t* s);
const char* service_status_name(service_t* s);
void        service_stop(service_t* s);
int         service_supervise(const char* service_dir_, const char* service, bool once);
void        service_update_dependency(service_t* s);
bool        service_is_dependency(service_t* s);
void        service_update_state(service_t* s, int state);
void        service_write(service_t* s);
