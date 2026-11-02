#!/bin/bash

CSV="results.csv"
echo "mode,threads,time" > $CSV

cleanup_shm() {
    ipcs -m | awk '/^0x/ {print $2}' | while read id; do
        ipcrm -m "$id" 2>/dev/null
    done
}

THREADS=(1 2 3 4 5 6 8 10 12 16)
MODES=(0 1)
REPEATS=10

for mode in "${MODES[@]}"; do
    mode_name=$([ "$mode" -eq 0 ] && echo "INDEPENDENT" || echo "SHARED")
    echo "========== $mode_name =========="

    for n in "${THREADS[@]}"; do
        for ((r=0; r<REPEATS; r++)); do
            cleanup_shm
            output=$(./build/worker "$n" "$mode" 2>&1)
            time_value=$(echo "$output" | grep -oP 'Time duration: \K[0-9.eE+-]+')
            echo "$mode_name,$n,$time_value" >> $CSV
        done
        echo "  $mode_name n=$n готово"
    done
done

cleanup_shm
echo "Результаты в $CSV"

if command -v python3 &>/dev/null && [ -f graphs.py ]; then
    python3 graphs.py
fi