#!/bin/bash

CSV="results.csv"
echo "method,file_size,buf_size,time,status" > $CSV

run_test() {
    local method=$1
    local file_label=$2
    local block_size=$3
    local block_count=$4
    local buf_size=$5

    rm -f input.txt output.txt
    dd if=/dev/urandom of=input.txt bs="$block_size" count="$block_count" status=none

    # Таймаут 60 секунд — если программа зависнет, убьём
    output=$(timeout 60 ./build/${method} "$buf_size" 2>&1)
    rc=$?

    time_value=$(echo "$output" | grep -oP 'Time duration: \K[0-9.]+')

    if [ $rc -eq 124 ]; then
        status="TIMEOUT"
    elif [ -f "output.txt" ]; then
        md5_in=$(md5sum input.txt | cut -d' ' -f1)
        md5_out=$(md5sum output.txt | cut -d' ' -f1)
        if [ "$md5_in" = "$md5_out" ]; then
            status="OK"
        else
            status="FAIL"
        fi
    else
        status="NO_OUTPUT"
    fi

    case "$method" in
        shared_mem)    csv_method="Shared memory" ;;
        message_queue) csv_method="Message queue" ;;
        named_pipe)    csv_method="FIFO" ;;
    esac

    echo "$csv_method,$file_label,$buf_size,${time_value:-0},$status" >> $CSV
    echo "  $csv_method $file_label buf=$buf_size → ${time_value:-?} [$status]"
}

for bin in shared_mem message_queue named_pipe; do
    [ ! -f "./build/$bin" ] && echo "Error: ./build/$bin not found!" && exit 1
done

BUF_SIZES=(8192 65536 1048576)
REPEATS=10 

declare -a FILES=(
    "8KB 8192 1"
    "4MB 1048576 4"
    "2GB 1048576 2048"
)

for method in shared_mem message_queue named_pipe; do
    echo "========== $method =========="
    for file_spec in "${FILES[@]}"; do
        read -r file_label bs bc <<< "$file_spec"
        for buf in "${BUF_SIZES[@]}"; do
            for ((r=0; r<REPEATS; r++)); do
                run_test $method "$file_label" "$bs" "$bc" "$buf"
            done
        done
    done
done

rm -f input.txt output.txt
