#!/bin/bash

[ -n "$2" ] && for obj in $(grep -E '^//\s+\+objects:' $1 | sed -E 's/\/\/\s+\+objects://'); do
  echo -n "$2/$obj "
done

grep -E '^//\s+\+flags:' $1 | sed -E "s/\/\/\s+\+flags://"
