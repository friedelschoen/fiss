# sigremap 8 "MAY 2023" "0.2.2" "fiss man page"

## NAME

`sigremap` - a minimal init system for Linux containers

## SYNOPSIS

`sigremap` \[option] \[old-signal=new-signal...] command \[arguments ...]

## DESCRIPTION

`sigremap` is a simple process supervisor that forwards signals to children. It is designed to run as PID1 in minimal container environments.

Optional arguments:

`-c, --single`
Run in single-child mode. In this mode, signals are only proxied to the direct child and not any of its descendants.

`-v, --verbose`
Print debugging information to stderr.

`-V, --version`
Print the current version and exit.

`old-signal=new-signal...`
Rewrite received signal s to new signal r before proxying. To ignore (not proxy) a signal, rewrite it to 0. This option can be specified multiple times.

## AUTHORS

`sigremap` is based on dumb-init by Yelp.
Rewritten by Friedel Schon <derfriedmundschoen@gmail.com>
