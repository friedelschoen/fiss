#pragma once

#include "service.h"


void  parse_env_file(char** env);
void  parse_param_file(service_t* s, char* args[]);
pid_t parse_pid_file(service_t* s);