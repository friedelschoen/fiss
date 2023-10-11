#pragma once

#include "service.h"

#include <unistd.h>


void parse_env_file(char** env);
void parse_param_file(struct service* s, char* args[]);
int  parse_ugid(char* str, uid_t* uid, gid_t* gids);
