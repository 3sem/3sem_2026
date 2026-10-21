#!/bin/bash
# cpu_nice.sh — сравнение CPU-времени для nice=0 и nice=19
set -e
DUR=10

# два одинаковых CPU-нагруженных процесса
yes > /dev/null & P1=$!
yes > /dev/null & P2=$!

# понижаем приоритет второму (только понижение без sudo)
renice 19 -p $P2 > /dev/null

sleep "$DUR"

# снимаем накопленное CPU-время до убийства
T1=$(ps -o time= -p $P1)
T2=$(ps -o time= -p $P2)
kill $P1 $P2 2>/dev/null

echo "nice=0  PID=$P1  CPU=$T1"
echo "nice=19 PID=$P2  CPU=$T2"
