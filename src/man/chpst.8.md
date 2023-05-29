# chpst 8 "MAY 2023" "%VERSION%" "fiss man page"

## NAME

`chpst` - runs a program with a changed process state

## SYNOPSIS

`chpst` \[`-vP012`] \[`-u` user] \[`-U` user] \[`-b` argv0] \[`-e` dir] \[`-/` root] \[`-n` inc] \[`-l`|`-L` lock] \[`-m` bytes] \[`-d` bytes] \[`-o` n] \[`-p` n] \[`-f` bytes] \[`-c` bytes] prog \[arguments...]

## DESCRIPTION

`-u [:]user[:group]`

setuidgid. Set uid and gid to the user's uid and gid, as found in /etc/passwd. If user is followed by a colon and a group, set the gid to group's gid, as found in /etc/group, instead of user's gid. If group consists of a colon-separated list of group names, chpst sets the group ids of all listed groups. If user is prefixed with a colon, the user and all group arguments are interpreted as uid and gids respectivly, and not looked up in the password or group file. All initial supplementary groups are removed.

`-U [:]user[:group]`

envuidgid. Set the environment variables $UID and $GID to the user's uid and gid, as found in /etc/passwd. If user is followed by a colon and a group, set $GID to the group's gid, as found in /etc/group, instead of user's gid. If user is prefixed with a colon, the user and group arguments are interpreted as uid and gid respectivly, and not looked up in the password or group file.

`-b argv0`
Run prog with argv0 as the 0th argument.

`-/ root`
Change the root directory to root before starting prog.

`-C pwd`
Change the working directory to pwd before starting prog. When combined with -/, the working directory is changed after the chroot.

`-n inc`
Add inc to the nice(2) value before starting prog. inc must be an integer, and may start with a minus or plus.

`-l lock`
Open the file lock for writing, and obtain an exclusive lock on it. lock will be created if it does not exist. If lock is locked by another process, wait until a new lock can be obtained.

`-L lock`
The same as -l, but fail immediately if lock is locked by another process.

`-P`
Run prog in a new process group.

`-0`
Close standard input before starting prog.

`-1`
Close standard output before starting prog.

`-2`
Close standard error before starting prog.


## NOT IMPLEMENTED

Following options are defined in runit's chpst but are ignored by fiss' implementation.

`-e dir`
Set various environment variables as specified by files in the directory dir: If dir contains a file named k whose first line is v, chpst removes the environment variable k if it exists, and then adds the environment variable k with the value v. The name k must not contain =. Spaces and tabs at the end of v are removed, and nulls in v are changed to newlines. If
the file k is empty (0 bytes long), chpst removes the environment variable k if it exists, without adding a new variable.

`-m bytes`
Limit the data segment, stack segment, locked physical pages, and total of all segment per process to bytes bytes each.

`-d bytes`
limit data segment. Limit the data segment per process to bytes bytes.

`-o n`
Limit the number of open file descriptors per process to n.

`-p n`
Limit the number of processes per uid to n.

`-f bytes`
Limit the output file size to bytes bytes.

`-c bytes`
Limit the core file size to bytes bytes.

`-v`
Print verbose messages to standard error. This includes warnings about limits unsupported by the system.


## EXIT CODES

chpst exits 100 when called with wrong options. It prints an error message and exits 111 if it has trouble changing the process state. Otherwise its exit code is the same as that of prog.

## AUTHOR

Based on the implementation by Gerrit Pape <pape@smarden.org>,
rewritten by Friedel Schon <derfriedmundschoen@gmail.com>
