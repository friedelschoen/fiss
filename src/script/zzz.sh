#!/bin/sh
# zzz - really simple suspend script

# Define usage information
USAGE="Usage: ${0##*/} [-nSzZR]
   -n   dry run (sleep for 5s instead of suspend/hibernate)
   -S   Low-power idle (ACPI S1)
   -z   suspend to RAM (ACPI S3) [DEFAULT for zzz(8)]
   -Z   hibernate to disk & power off (ACPI S4) [DEFAULT for ZZZ(8)]
   -R   hibernate to disk & reboot
   -H   hibernate to disk & suspend (aka suspend-hybrid)"

# Define a function to print error messages and exit with error code 1
fail() {
  echo "${0##*/}: $*" >&2
  exit 1
}

# Set default values for environment variables
export ZZZ_MODE=suspend
export ZZZ_HIBERNATE_MODE=platform

# Check the name of the script to determine the default mode
case "$0" in
  *ZZZ) ZZZ_MODE=hibernate;;
esac

# Parse command-line options
while getopts hnSzHRZ opt; do
  case "$opt" in
    n) ZZZ_MODE=noop;;
    S) ZZZ_MODE=standby;;
    z) ZZZ_MODE=suspend;;
    Z) ZZZ_MODE=hibernate;;
    R) ZZZ_MODE=hibernate; ZZZ_HIBERNATE_MODE=reboot;;
    H) ZZZ_MODE=hibernate; ZZZ_HIBERNATE_MODE=suspend;;
    [h?]) fail "$USAGE";;
  esac
done

# Shift command-line arguments to skip processed options
shift $((OPTIND-1))

# Check if the selected mode is supported
case "$ZZZ_MODE" in
  suspend) grep -q mem /sys/power/state || fail "suspend not supported";;
  hibernate) grep -q disk /sys/power/state || fail "hibernate not supported";;
esac

# Check if the current user has permission to sleep the system
test -w /sys/power/state || fail "sleep permission denied"

# Run the main logic in a subshell with a file lock to prevent multiple instances
(
  flock -n 9 || fail "another instance of zzz is running"

  printf "Zzzz... "

  # Run suspend hooks
  for hook in /etc/zzz.d/suspend/*; do
    [ -x "$hook" ] && "$hook"
  done

  # Sleep the system according to the selected mode
  case "$ZZZ_MODE" in
    standby) printf freeze >/sys/power/state || fail "standby failed";;
    suspend) printf mem >/sys/power/state || fail "suspend failed";;
    hibernate)
      echo $ZZZ_HIBERNATE_MODE >/sys/power/disk
      printf disk >/sys/power/state || fail "hibernate failed"
      ;;
    noop) sleep 5;;
  esac

  # Run resume hooks
  for hook in /etc/zzz.d/resume/*; do
    [ -x "$hook" ] && "$hook"
  done

  echo "yawn."
) 9</sys/power
