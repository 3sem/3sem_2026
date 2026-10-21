#!/bin/bash
# io_ionice.sh — idle-класс I/O не мешает best-effort
set -e
F=/tmp/io_test.bin
[ -f "$F" ] || dd if=/dev/urandom of="$F" bs=1M count=256 status=none

# фоновая нагрузка
( while :; do cat "$F" > /dev/null; done ) & BG=$!
sleep 1

# замер: best-effort с низким приоритетом vs idle
t_best=$( { /usr/bin/time -f %e ionice -c2 -n7 cat "$F" > /dev/null; } 2>&1 )
t_idle=$( { /usr/bin/time -f %e ionice -c3    cat "$F" > /dev/null; } 2>&1 )

kill $BG 2>/dev/null; wait 2>/dev/null

echo "best-effort(-n7): ${t_best}s"
echo "idle(-c3):        ${t_idle}s"
