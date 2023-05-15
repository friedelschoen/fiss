#include "user_group.h"

#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>


/* uid:gid[:gid[:gid]...] */
static int parse_ugid_num(char* str, uid_t* uid, gid_t* gids) {
	int i;

	char* end;
	*uid = strtoul(str, &end, 10);

	if (*end != ':')
		return -1;

	str = end + 1;
	for (i = 0; i < 60; ++i, ++str) {
		gids[i++] = strtoul(str, &end, 10);

		if (*end != ':')
			break;

		str = end + 1;
	}

	if (*str != '\0')
		return -1;

	return i;
}

int parse_ugid(char* str, uid_t* uid, gid_t* gids) {
	struct passwd* pwd;
	struct group*  gr;
	char*          end;
	char*          groupstr = NULL;
	int            gid_size = 0;

	if (str[0] == ':')
		return parse_ugid_num(str + 1, uid, gids);

	if ((end = strchr(str, ':')) != NULL) {
		end[0]   = '\0';
		groupstr = end + 1;
	}

	if ((pwd = getpwnam(str)) == NULL) {
		return -1;
	}
	*uid = pwd->pw_uid;

	if (groupstr == NULL) {
		gids[0] = pwd->pw_gid;
		return 1;
	}

	char* next = groupstr;

	while (next && gid_size < 60) {
		groupstr = next;
		if ((end = strchr(groupstr, ':')) != NULL) {
			end[0] = '\0';
			next   = end + 1;
		} else {
			next = NULL;
		}
		if ((gr = getgrnam(groupstr)) == NULL)
			return -1;

		gids[gid_size++] = gr->gr_gid;
	}

	return gid_size;
}
