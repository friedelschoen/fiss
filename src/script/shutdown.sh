#!/bin/sh
# shutdown - shutdown(8) lookalike for runit

single() {
  fsvc chlevel single
}

abort() {
  printf '%s\n' "$1" >&2
  exit 1
}

usage() {
  abort "Usage: ${0##*/} [-fF] [-kchPr] time [warning message]"
}

action=single

while getopts akrhPHfFnct: opt; do
  case "$opt" in
    a|n|H) abort "'-$opt' is not implemented";;
    t) ;;
    f) touch /fastboot;;
    F) touch /forcefsck;;
    k) action=true;;
    c) action=cancel;;
    h|P) action=halt;;
    r) action=reboot;;
    [?]) usage;;
  esac
done
shift $((OPTIND - 1))

[ $# -eq 0 ] && usage

time=$1; shift
message="${*:-system is going down}"

if [ "$action" = "cancel" ]; then
  kill "$(cat /run/fiss/shutdown.pid)"
  if [ -e /etc/nologin ] && ! [ -s /etc/nologin ]; then
    rm /etc/nologin
  fi
  echo "${*:-shutdown cancelled}" | wall
  exit
fi

touch /run/fiss/shutdown.pid 2>/dev/null || abort "Not enough permissions to execute ${0#*/}"
echo $$ >/run/fiss/shutdown.pid

case "$time" in
  now) time=0;;
  +*) time=${time#+};;
  *:*) abort "absolute time is not implemented";;
  *) abort "invalid time";;
esac

for break in 5 0; do
  [ "$time" -gt "$break" ] || continue
  [ "$break" = 0 ] && touch /etc/nologin

  printf '%s in %s minutes\n' "$message" "$time" | wall
  printf 'shutdown: sleeping for %s minutes... ' "$(( time - break ))"
  sleep $(( (time - break) * 60 ))
  time="$break"
  printf '\n'

  [ "$break" = 0 ] && rm /etc/nologin
done

printf '%s NOW\n' "$message" | wall

$action
