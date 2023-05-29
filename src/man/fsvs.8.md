# fsvs 8 "MAY 2023" "%VERSION%" "fiss man page"

## NAME

`fsvs` - friedel's service superviser

## SYNOPSIS

`fsvs` \[options] service-dir runlevel

## DESCRIPTION

`fsvs` is the superviser called by `finit` but without initialization.

Following options are available:

`-v, --verbose`
Prints more information, easier to debug problems

`-f, --force`
Forces the socket if existing, if called without `-f` and the socket already exists, it failes.

`-V, --version`
Prints the version and exits.

Services can be controlled with `fsvc 8`

## AUTHORS

Friedel Schon <derfriedmundschoen@gmail.com>
