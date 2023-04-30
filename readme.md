# FISS (Friedel's Initialization and Service Supervision)

> 0.1.0 (May 2023)

FISS is a lightweight and easy-to-use tool for initializing and supervising long-running services on Unix-like systems. It provides a simple and reliable way to start, stop, and monitor services. It supports automatic restarts, logging, and customizable startup options.

FISS is inspired by tools like [runit](http://smarden.org/runit/) and [daemontools](http://cr.yp.to/daemontools.html), but it aims to be simpler and more flexible, while still providing a similar level of reliability and security.

## Features

FISS provides the following features:

-   Automatic restarts: If a service exits unexpectedly, FISS will automatically restart it, up to a configurable number of times.
-   Logging: FISS captures the stdout and stderr of the service and writes them to log files, which can be rotated and compressed.
-   Customizable startup options: FISS allows you to specify environment variables, working directory, umask, and other options for the service.
-   Supervision: FISS monitors the service and ensures that it stays running, or else it terminates the service and retries later.
-   Status monitoring: FISS provides a simple command-line interface to check the status of a service, including its uptime, PID, and exit code.
-   Simple configuration: FISS uses a simple directory structure to store the configuration for each service, making it easy to manage and version control.

## Dependencies

-   a POSIX shell compliment like `dash` or `bash`
-   `awk`

## Installation

FISS is written in C and POSIX Shell. To compile this project, the following tools are required:

-   `gcc` (or a gcc-compatible compiler like `clang`)
-   `make`
-   [`md2man-*`](https://github.com/sunaku/md2man) to make the manuals

To compile the project simply run:

```sh
$ make           # to compile everything
$ make binary    # to compile just the executables
$ make manual    # to compile the manuals
```

Then copy the contents of `bin/` to `/sbin/`, `usr/*` to `/usr/` and `etc/` to `/etc/`, the manuals are in `man`, copy them to `/usr/share/man/man8`.

## Usage

To use FISS, you need to create a configuration directory for each service you want to supervise.

A service directory must contain either of these files: `run`, `start`, `depends`

```
/etc/service.d/my-service/
├── log/
├── nolog
├── log
├── depends
├── env
├── finish
├── params
├── pid
├── run
├── setup
├── start
├── stop
├── user
├── <runlevel>-once
└── <runlevel>-up
```

### `run`

The `run` file should be marked as executable and is meant be a long-running service. This can be a symbolic link to an executable or a simple script that executes.

It must not coexist with `start`.

```sh
#!/bin/sh

USER=hello
PASSWORD=world

exec python3 my-service.py $USER $PASSWORD
```

### `start`

This file initiates a long-running background-service. This can be a symbolic link to an executable or a simple script.

It must not coexist with `run`.

If `start` exist, either `stop` or `pid` must exist to let fiss terminate a service, the function of those files is described beneath. `pid` is stat and read after `start` exits.

If your service provides an foreground-running interface, it's advised to use this in combination with `run` as it's easier to supervise a foreground-process!

```sh
#!/bin/sh

myservice --daemon > ./pid   # myservice prints it's PID to stdout
```

### `stop`

This file is meant to stop a background-service and will be executed whenever a service should terminate a background service.

It may not coexist with pid.

```sh
#!/bin/sh

myservice --stop
```

### `pid`

This file contains the pid of a background service in decimal.

### `depends`

This file contains a newline-terminated list of services that should be running before this service is started. A service can only contain this file, it will be considered a meta-service.

```
dbus
NetworkManager
```

### `nolog`

By default FISS logs all services without a `log` file or directory inside _/var/log/fiss/\<service>.log_. If this file is present, this services will not be logged.

This is useful for very verbose daemons, which would cause gigabytes of log-files.

### `log/` directory

If `log` is a directory, it should contain a service, the output or the service (if running as a foreground-service) will be redirected to the `run` of your log-service.

The log-service is a dependency of your service. If the service or the log-service unexpectedly terminates, no data will be lost as data is buffered.

### `log` file

Output of the service is redirected to this file if existing. This can be used for simple services with no time-specific output as nothing but the output is redirected.

If formatting or time-prefixed are wished, consider using a log-service.

### `user`

If existing, `start` of `run` will be executed as this user (and optionally a group). The contents should be in format `<user>[:<group>:<group...>]` or `<uid>:<gid>[:<gid...>]`

```
fiss-user:fiss-user
```

### `env`

If existing, environment-variables will be passed to `run` or `start`. This is a simple `key=value`-file with **no variable-substitution**.

```
HOME=/home/fiss-user/
THREADS=16
BE_COOL=yes
```

### `params`

If existing, arguments are passed to `run` or `start`. Every line is considered an argument and **no variable-substitution** is applied.

If the first line starts with a `%` (percent sign), the 0th argument is set, otherwise this list begins with the first argument and `argv[0]` stays `'./run'`

```
%mysql
-u
cool user
-p
-d
```

which is equivalent to the shell-call:

```
mysql -u 'cool user' -p -d
```

### `setup`

This file is executed before `start` or `run` is called with an unchanged environment (thus `root`). It's useful for e.g. creating files which is not permitted by the user specified in `user`.

```sh
#!/bin/sh

mkdir /run/myservice/
chmod a+rw /run/service
```

### `finish`

This file is executed after `run` or `stop` with an unchanged environment (thus `root`). It's useful for e.g. cleaning up files which is not permitted by the user specified in `user`.

```sh
#!/bin/sh

rmdir /run/myservice/
```

### `<runlevel>-up` and `<runlevel>-once`

If this file is present, this service is started if `finit` is running with specified `runlevel`. If `-up` is present, this service will be restarted whenever it terminated. If `-once` is present, it will be started once and not restarted on exiting.

Both `<runlevel>-up` and `<runlevel>-once` must not coexist.

## Controlling

To control fiss (e.g. `finit` and `fsvs`) you use `fsvc` (friedel's service controller).

For further information check out [the manual](man/fsvc.8.md).

## Licensing

This project is licensed under the terms of the [zlib license](./LICENSE).
