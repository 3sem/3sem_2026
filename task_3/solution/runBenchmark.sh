#!/usr/bin/env bash
set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
export LC_ALL=C

repetitions=${1-10}
file_size=${2-67108864}
if [[ $# -gt 2 || ! $repetitions =~ ^[1-9][0-9]*$ || ! $file_size =~ ^[1-9][0-9]*$ ]]; then
    echo "Usage: $0 [positive_repetition_count] [fixed_file_size_in_bytes]" >&2
    exit 1
fi

result_dir=benchmark
mkdir -p -- "$result_dir"
printf 'type,file_size,chunk_size,run,real_seconds,user_seconds,sys_seconds\n' > "$result_dir/results.csv"

sender_pid=""
reader_pid=""

cleanup(){
    if [[ -n $reader_pid ]]; then
        kill "$reader_pid" 2>/dev/null || true
        wait "$reader_pid" 2>/dev/null || true
    fi
    if [[ -n $sender_pid ]]; then
        kill "$sender_pid" 2>/dev/null || true
        wait "$sender_pid" 2>/dev/null || true
    fi
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

wait_for_sender(){
    local transfer_mode=$1
    local deadline=$((SECONDS + 5))
    local wait_channel=""

    while ((SECONDS < deadline)); do
        if [[ $transfer_mode == queue ]]; then
            if awk -v pid="$sender_pid" '
                NR == 1 {
                    for(i = 1; i <= NF; i++) if($i == "lspid") column = i
                    next
                }
                column && $column == pid { found = 1 }
                END { exit !found }
            ' /proc/sysvipc/msg; then
                return 0
            fi
        else
            wait_channel=$(cat "/proc/$sender_pid/wchan" 2>/dev/null || true)
            if [[ $wait_channel == *futex* ]]; then
                return 0
            fi
        fi

        if [[ $transfer_mode != queue ]] && ! kill -0 "$sender_pid" 2>/dev/null; then
            echo "Sender exited before IPC was ready. See $result_dir/sender.log." >&2
            return 1
        fi
        sleep 0.001
    done

    echo "IPC readiness timeout. Check sender.log and access to /proc." >&2
    return 1
}

run_transfer(){
    local transfer_mode=$1
    local chunk_size=$2
    local deadline=0

    : > "$result_dir/reader.log"

    bash ./transfer.sh "$transfer_mode" send --chunk-size "$chunk_size" --no-prepare --no-check \
        > "$result_dir/sender.log" 2>&1 &
    sender_pid=$!

    if [[ $transfer_mode != fifo ]]; then
        wait_for_sender "$transfer_mode" || return 1
    fi

    timeout 30s bash ./transfer.sh "$transfer_mode" read --chunk-size "$chunk_size" --no-prepare --no-check \
        > "$result_dir/reader.log" 2>&1 &
    reader_pid=$!

    wait "$reader_pid" || return 1
    reader_pid=""

    deadline=$((SECONDS + 5))
    while kill -0 "$sender_pid" 2>/dev/null; do
        if ((SECONDS >= deadline)); then
            echo "Sender did not finish after the receiver exited." >&2
            return 1
        fi
        sleep 0.001
    done
    wait "$sender_pid" || return 1
    sender_pid=""
}

TIMEFORMAT='%R,%U,%S'

# Reuse the same input file and reference checksum for every measurement.
bash ./genTestFile.sh testFile "$file_size"
input_checksum=$(md5sum < testFile)

for transfer_mode in fifo shMem queue; do
    bash ./buildTransfer.sh "$transfer_mode"

    sudo sysctl -w kernel.msgmax=268435456
    sudo sysctl -w kernel.msgmnb=1073741824

    for chunk_size in $((8 * 1024)) $((64 * 1024)) $((1024 * 1024)) \
                      $((8 * 1024 * 1024)) $((64 * 1024 * 1024)); do
        for ((run = 0; run <= repetitions; run++)); do
            # Run 0 is a warm-up and is not written to results.csv.
            bash ./prepareTransfer.sh "$transfer_mode" read
            rm -f -- testFileOut

            echo "Testing $transfer_mode: file $file_size bytes, chunk $chunk_size bytes, run $run/$repetitions"
            if { time run_transfer "$transfer_mode" "$chunk_size"; } 2> "$result_dir/timing.txt"; then
                output_checksum=$(md5sum < testFileOut)
                if [[ ${input_checksum%% *} != "${output_checksum%% *}" ]]; then
                    echo "MD5 mismatch; measurement rejected. Files preserved." >&2
                    exit 1
                fi
            else
                cat "$result_dir/timing.txt" "$result_dir/sender.log" >&2
                if [[ -f $result_dir/reader.log ]]; then
                    cat "$result_dir/reader.log" >&2
                fi
                exit 1
            fi

            if ((run > 0)); then
                printf '%s,%s,%s,%s,%s\n' "$transfer_mode" "$file_size" "$chunk_size" "$run" \
                    "$(cat "$result_dir/timing.txt")" >> "$result_dir/results.csv"
            fi
            rm -f -- testFileOut
        done
    done
done

rm -f -- testFile

echo "Results saved to $result_dir/results.csv"
python3 ./plotBenchmark.py "$result_dir/results.csv" --output "$result_dir/transfer_times.png"
