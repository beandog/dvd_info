#!/bin/bash

# Accept first argument as DVD device, or use /dev/dvd as default
device=$1
if [[ -z "$1" ]]; then
  device=/dev/dvd
fi

# Run dvd_drive_status, and dump all output to /dev/null for silence
dvd_drive_status &> /dev/null

# Get the exit code
exit_code=$?

# Quit if it's already ready already! :)
if [[ $exit_code -ne 3 ]]; then
  exit 0
fi

# Otherwise, wait patiently (one second) and try again, until it is ready
until [[ $exit_code -ne 3 ]]; do
  dvd_drive_status &> /dev/null
  exit_code=$?
  sleep 1
done
