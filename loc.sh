#!/bin/bash
# Count lines of code in .c and .h files recursively from current directory

find . -type f \( -name "*.c" -o -name "*.h" \) -print0 | \
  xargs -0 wc -l | \
  tee /dev/stderr | \
  tail -n 1
