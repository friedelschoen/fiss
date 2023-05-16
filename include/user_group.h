#pragma once

#include <unistd.h>


int parse_ugid(char* str, uid_t* uid, gid_t* gids);
