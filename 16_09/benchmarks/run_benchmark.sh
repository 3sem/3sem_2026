#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C
TIMEFORMAT='%R,%U,%S'

if [[ $# -ne 9 ]]; then
    echo "Usage: $0 <shm_sender> <shm_receiver> <queue_sender> <queue_receiver> <fifo_sender> <fifo_receiver> <output_dir> <python> <md5sum>" >&2
    exit 2
fi

SHM_SENDER=$(realpath "$1")
SHM_RECEIVER=$(realpath "$2")
QUEUE_SENDER=$(realpath "$3")
QUEUE_RECEIVER=$(realpath "$4")
FIFO_SENDER=$(realpath "$5")
FIFO_RECEIVER=$(realpath "$6")
OUTPUT_DIR=$7
PYTHON=$8
MD5SUM=$9

FILE_SIZE_BYTES=${FILE_SIZE_BYTES:-67108864}
REPEATS=${REPEATS:-5}
WARMUPS=${WARMUPS:-1}
READY_TIMEOUT=${READY_TIMEOUT:-15}
TRANSFER_TIMEOUT=${TRANSFER_TIMEOUT:-120}

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)

mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR=$(cd "$OUTPUT_DIR" && pwd)
WORK_ROOT="$OUTPUT_DIR/work"
INPUT_FILE="$OUTPUT_DIR/input.bin"
CSV_FILE="$OUTPUT_DIR/results.csv"
mkdir -p "$WORK_ROOT"

for command_name in ipcs ipcrm awk timeout ps head realpath sleep sudo shuf; do
    if ! command -v "$command_name" >/dev/null 2>&1; then
        echo "Required command not found: $command_name" >&2
        exit 1
    fi
done

if [[ ! "$FILE_SIZE_BYTES" =~ ^[1-9][0-9]*$ ]] ||
   [[ ! "$REPEATS" =~ ^[1-9][0-9]*$ ]] ||
   [[ ! "$WARMUPS" =~ ^[0-9]+$ ]] ||
   [[ ! "$READY_TIMEOUT" =~ ^[1-9][0-9]*$ ]] ||
   [[ ! "$TRANSFER_TIMEOUT" =~ ^[1-9][0-9]*$ ]]; then
    echo "FILE_SIZE_BYTES, REPEATS, WARMUPS and timeouts must be valid integers" >&2
    exit 1
fi

# Generate the same input file for every measurement.
head -c "$FILE_SIZE_BYTES" /dev/urandom > "$INPUT_FILE"
INPUT_MD5=$("$MD5SUM" "$INPUT_FILE" | awk '{print $1}')

PIPE_SIZE=$("$PYTHON" - <<'PY'
import fcntl
import os

read_fd, write_fd = os.pipe()
try:
    print(fcntl.fcntl(write_fd, fcntl.F_GETPIPE_SZ))
finally:
    os.close(read_fd)
    os.close(write_fd)
PY
)

# Calculate QueueMessage metadata using the native C ABI layout.
QUEUE_METADATA_SIZE=$("$PYTHON" - <<'PY'
import ctypes

class QueueMessageHeader(ctypes.Structure):
    _fields_ = [
        ("mtype", ctypes.c_long),
        ("size", ctypes.c_size_t),
        ("finished", ctypes.c_bool),
        ("data", ctypes.c_char * 0),
    ]

print(QueueMessageHeader.data.offset - ctypes.sizeof(ctypes.c_long))
PY
)

SMALL_SIZE=${SMALL_BUFFER_SIZE:-8128}
if [[ ! "$SMALL_SIZE" =~ ^[1-9][0-9]*$ ]]; then
    echo "SMALL_BUFFER_SIZE must be a positive integer" >&2
    exit 1
fi

MEDIUM_SIZE=$PIPE_SIZE
if (( MEDIUM_SIZE <= SMALL_SIZE )); then
    MEDIUM_SIZE=$((SMALL_SIZE * 2))
fi

LARGE_SIZE=$((PIPE_SIZE * 4))
if (( LARGE_SIZE < 1024 * 1024 )); then
    LARGE_SIZE=$((1024 * 1024))
fi
if (( LARGE_SIZE <= MEDIUM_SIZE )); then
    LARGE_SIZE=$((MEDIUM_SIZE * 4))
fi

# Save the original limits. Restore them on normal exit or a catchable signal
# if this script had to change them. Never lower a limit that was already high.
ORIGINAL_MSG_MAX=$(< /proc/sys/kernel/msgmax)
ORIGINAL_MSGMNB=$(< /proc/sys/kernel/msgmnb)
CHANGED_MSG_MAX=0
CHANGED_MSGMNB=0
SET_MSG_MAX=$ORIGINAL_MSG_MAX
SET_MSGMNB=$ORIGINAL_MSGMNB

restoreQueueLimits() {
    local exit_status=$?
    trap - EXIT

    if (( CHANGED_MSGMNB == 1 )); then
        local current_msgmnb
        current_msgmnb=$(< /proc/sys/kernel/msgmnb)
        if [[ "$current_msgmnb" == "$SET_MSGMNB" ]]; then
            if ! sudo sysctl -w "kernel.msgmnb=$ORIGINAL_MSGMNB" >/dev/null; then
                echo "Warning: could not restore kernel.msgmnb=$ORIGINAL_MSGMNB" >&2
            fi
        else
            echo "Warning: kernel.msgmnb changed externally; leaving its current value unchanged" >&2
        fi
    fi

    if (( CHANGED_MSG_MAX == 1 )); then
        local current_msgmax
        current_msgmax=$(< /proc/sys/kernel/msgmax)
        if [[ "$current_msgmax" == "$SET_MSG_MAX" ]]; then
            if ! sudo sysctl -w "kernel.msgmax=$ORIGINAL_MSG_MAX" >/dev/null; then
                echo "Warning: could not restore kernel.msgmax=$ORIGINAL_MSG_MAX" >&2
            fi
        else
            echo "Warning: kernel.msgmax changed externally; leaving its current value unchanged" >&2
        fi
    fi

    exit "$exit_status"
}

# One large queue message must fit msgmax and msgmnb. The queue byte limit is
# set several times larger so the sender and receiver can overlap in practice.
REQUIRED_MSG_MAX=$((LARGE_SIZE + QUEUE_METADATA_SIZE + 4096))
REQUIRED_MSGMNB=$((REQUIRED_MSG_MAX * 4))
if (( REQUIRED_MSGMNB < 8 * 1024 * 1024 )); then
    REQUIRED_MSGMNB=$((8 * 1024 * 1024))
fi

if (( ORIGINAL_MSG_MAX < REQUIRED_MSG_MAX || ORIGINAL_MSGMNB < REQUIRED_MSGMNB )); then
    sudo -v
    trap restoreQueueLimits EXIT

    if (( ORIGINAL_MSG_MAX < REQUIRED_MSG_MAX )); then
        CHANGED_MSG_MAX=1
        SET_MSG_MAX=$REQUIRED_MSG_MAX
        echo "Temporarily increasing kernel.msgmax to $SET_MSG_MAX bytes."
        sudo sysctl -w "kernel.msgmax=$SET_MSG_MAX"
    fi

    if (( ORIGINAL_MSGMNB < REQUIRED_MSGMNB )); then
        CHANGED_MSGMNB=1
        SET_MSGMNB=$REQUIRED_MSGMNB
        echo "Temporarily increasing kernel.msgmnb to $SET_MSGMNB bytes."
        sudo sysctl -w "kernel.msgmnb=$SET_MSGMNB"
    fi
fi

MSG_MAX=$(< /proc/sys/kernel/msgmax)
MSG_QUEUE_MAX=$(< /proc/sys/kernel/msgmnb)
MESSAGE_LIMIT=$MSG_MAX
if (( MSG_QUEUE_MAX < MESSAGE_LIMIT )); then
    MESSAGE_LIMIT=$MSG_QUEUE_MAX
fi
if (( MESSAGE_LIMIT <= QUEUE_METADATA_SIZE )); then
    echo "System V queue limits are too small for the message metadata" >&2
    exit 1
fi
QUEUE_PAYLOAD_LIMIT=$((MESSAGE_LIMIT - QUEUE_METADATA_SIZE))
if (( QUEUE_PAYLOAD_LIMIT < LARGE_SIZE )); then
    echo "Configured queue limits do not permit a message of $LARGE_SIZE bytes" >&2
    exit 1
fi

echo "Input size: $FILE_SIZE_BYTES bytes"
echo "Application buffers: small=$SMALL_SIZE, medium=$MEDIUM_SIZE, large=$LARGE_SIZE"
echo "System V queue limits during test: msgmax=$MSG_MAX, msgmnb=$MSG_QUEUE_MAX"
echo "Queue payload limit: $QUEUE_PAYLOAD_LIMIT bytes"

ftok_key() {
    "$PYTHON" - "$1" "$2" <<'PY'
import ctypes
import os
import sys

libc = ctypes.CDLL(None, use_errno=True)
libc.ftok.argtypes = [ctypes.c_char_p, ctypes.c_int]
libc.ftok.restype = ctypes.c_int
key = libc.ftok(os.fsencode(sys.argv[1]), int(sys.argv[2]))
if key == -1:
    sys.exit("ftok failed")
print(f"0x{key & 0xffffffff:08x}")
PY
}

remove_shm_ipc() {
    local shm_key=$1
    local sem_key=$2
    ipcrm -M "$shm_key" >/dev/null 2>&1 || true
    ipcrm -S "$sem_key" >/dev/null 2>&1 || true
}

remove_queue_ipc() {
    ipcrm -Q "$1" >/dev/null 2>&1 || true
}

shared_memory_exists() {
    ipcs -m | awk -v key="$1" 'tolower($1) == tolower(key) { found = 1 } END { exit !found }'
}

semaphore_set_exists() {
    ipcs -s | awk -v key="$1" 'tolower($1) == tolower(key) { found = 1 } END { exit !found }'
}

message_queue_exists() {
    ipcs -q | awk -v key="$1" 'tolower($1) == tolower(key) { found = 1 } END { exit !found }'
}

wait_for_shm_ipc() {
    local shm_key=$1
    local sem_key=$2
    local sender_pid=$3
    local deadline=$((SECONDS + READY_TIMEOUT))
    local state

    while (( SECONDS < deadline )); do
        if shared_memory_exists "$shm_key" && semaphore_set_exists "$sem_key"; then
            return 0
        fi
        state=$(ps -o stat= -p "$sender_pid" 2>/dev/null || true)
        if [[ -z "$state" || "$state" == Z* ]]; then
            return 1
        fi
        sleep 0.001
    done

    echo "Timed out waiting for shared memory and semaphores" >&2
    return 1
}

wait_for_queue() {
    local queue_key=$1
    local sender_pid=$2
    local deadline=$((SECONDS + READY_TIMEOUT))
    local state

    while (( SECONDS < deadline )); do
        if message_queue_exists "$queue_key"; then
            return 0
        fi
        state=$(ps -o stat= -p "$sender_pid" 2>/dev/null || true)
        if [[ -z "$state" || "$state" == Z* ]]; then
            return 1
        fi
        sleep 0.001
    done

    echo "Timed out waiting for message queue" >&2
    return 1
}

run_transfer() (
    local mechanism=$1
    local work_dir=$2
    local buffer_size=$3
    local output_file=$4
    local shm_key=$5
    local sem_key=$6
    local queue_key=$7
    local sender receiver
    local sender_pid=""
    local receiver_pid=""

    case "$mechanism" in
        sysv_shm)
            sender=$SHM_SENDER
            receiver=$SHM_RECEIVER
            ;;
        sysv_queue)
            sender=$QUEUE_SENDER
            receiver=$QUEUE_RECEIVER
            ;;
        fifo)
            sender=$FIFO_SENDER
            receiver=$FIFO_RECEIVER
            ;;
        *)
            echo "Unknown mechanism: $mechanism" >&2
            exit 2
            ;;
    esac

    cd "$work_dir" || exit 1

    cleanup_on_exit() {
        local status=$?
        trap - EXIT INT TERM

        if [[ -n "$receiver_pid" ]] && kill -0 "$receiver_pid" 2>/dev/null; then
            kill "$receiver_pid" 2>/dev/null || true
            wait "$receiver_pid" 2>/dev/null || true
        fi
        if [[ -n "$sender_pid" ]] && kill -0 "$sender_pid" 2>/dev/null; then
            kill "$sender_pid" 2>/dev/null || true
            wait "$sender_pid" 2>/dev/null || true
        fi

        if (( status != 0 )); then
            case "$mechanism" in
                sysv_shm) remove_shm_ipc "$shm_key" "$sem_key" ;;
                sysv_queue) remove_queue_ipc "$queue_key" ;;
                fifo) rm -f -- "$work_dir/transfer.fifo" ;;
            esac
        fi
        exit "$status"
    }
    trap cleanup_on_exit EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM

    "$sender" "$INPUT_FILE" "$buffer_size" > sender.log 2>&1 &
    sender_pid=$!

    case "$mechanism" in
        sysv_shm)
            wait_for_shm_ipc "$shm_key" "$sem_key" "$sender_pid" || {
                echo "Sender failed or shared-memory initialization timed out" > run_error.log
                cat sender.log >&2 || true
                exit 1
            }
            ;;
        sysv_queue)
            wait_for_queue "$queue_key" "$sender_pid" || {
                echo "Sender failed or queue initialization timed out" > run_error.log
                cat sender.log >&2 || true
                exit 1
            }
            ;;
        fifo)
            # The sender blocks in open(O_WRONLY) until the receiver opens FIFO.
            ;;
    esac

    if [[ "$mechanism" == "fifo" ]]; then
        timeout "${TRANSFER_TIMEOUT}s" "$receiver" \
        "$output_file" "$buffer_size" > receiver.log 2>&1 &
    else
        timeout "${TRANSFER_TIMEOUT}s" "$receiver" \
        "$output_file" > receiver.log 2>&1 &
    fi

    receiver_pid=$! 

    if wait "$receiver_pid"; then
        receiver_pid=""
    else
        local status=$?
        receiver_pid=""
        echo "Receiver exited with status $status" > run_error.log
        exit 1
    fi

    if wait "$sender_pid"; then
        sender_pid=""
    else
        local status=$?
        sender_pid=""
        echo "Sender exited with status $status" > run_error.log
        exit 1
    fi
)

# A scenario is mechanism + application-buffer case. Each round is shuffled
# so that all methods are tested in varying system/cache conditions.
BUFFER_CASES=("small:$SMALL_SIZE" "medium:$MEDIUM_SIZE" "large:$LARGE_SIZE")
MECHANISMS=(sysv_shm sysv_queue fifo)
SCENARIOS=()
for mechanism in "${MECHANISMS[@]}"; do
    for buffer_case in "${BUFFER_CASES[@]}"; do
        case_name=${buffer_case%%:*}
        buffer_size=${buffer_case#*:}
        effective_chunk_size=$buffer_size
        if [[ "$mechanism" == "sysv_queue" ]] && (( effective_chunk_size > QUEUE_PAYLOAD_LIMIT )); then
            effective_chunk_size=$QUEUE_PAYLOAD_LIMIT
        fi
        SCENARIOS+=("$mechanism|$case_name|$buffer_size|$effective_chunk_size")
    done
done

printf '%s\n' \
    "mechanism,buffer_case,buffer_size,effective_chunk_size,file_size_bytes,run,real_seconds,user_seconds,system_seconds,input_md5,output_md5" \
    > "$CSV_FILE"

TOTAL_ROUNDS=$((WARMUPS + REPEATS))
for ((round_index = 0; round_index < TOTAL_ROUNDS; ++round_index)); do
    mapfile -t SHUFFLED_SCENARIOS < <(printf '%s\n' "${SCENARIOS[@]}" | shuf)
    if (( round_index < WARMUPS )); then
        echo
        echo "Warm-up round $((round_index + 1))/$WARMUPS (randomized order)"
    else
        echo
        echo "Measurement round $((round_index - WARMUPS + 1))/$REPEATS (randomized order)"
    fi

    for scenario in "${SHUFFLED_SCENARIOS[@]}"; do
        IFS='|' read -r mechanism case_name buffer_size effective_chunk_size <<< "$scenario"
        echo "Testing $mechanism: $case_name, app buffer=$buffer_size, effective chunk=$effective_chunk_size"

        work_dir=$(mktemp -d "$WORK_ROOT/${mechanism}_${case_name}_XXXXXX")
        : > "$work_dir/ipc.key"

        shm_key=$(ftok_key "$work_dir/ipc.key" 77)
        sem_key=$(ftok_key "$work_dir/ipc.key" 83)
        queue_key=$(ftok_key "$work_dir/ipc.key" 81)

        remove_shm_ipc "$shm_key" "$sem_key"
        remove_queue_ipc "$queue_key"
        rm -f -- "$work_dir/transfer.fifo"

        output_file="$work_dir/output.bin"
        timing_file="$work_dir/timing.txt"

        if { time run_transfer "$mechanism" "$work_dir" "$buffer_size" \
            "$output_file" "$shm_key" "$sem_key" "$queue_key"; } 2> "$timing_file"; then
            IFS=, read -r real_seconds user_seconds system_seconds < "$timing_file"
        else
            remove_shm_ipc "$shm_key" "$sem_key"
            remove_queue_ipc "$queue_key"
            rm -f -- "$work_dir/transfer.fifo"
            echo "Transfer failed. Logs retained at: $work_dir" >&2
            cat "$work_dir/run_error.log" 2>/dev/null || true
            cat "$work_dir/sender.log" 2>/dev/null || true
            cat "$work_dir/receiver.log" 2>/dev/null || true
            cat "$timing_file" >&2
            exit 1
        fi

        remove_shm_ipc "$shm_key" "$sem_key"
        remove_queue_ipc "$queue_key"
        rm -f -- "$work_dir/transfer.fifo"

        if [[ ! -f "$output_file" ]]; then
            echo "Output file was not created: $work_dir" >&2
            exit 1
        fi

        output_md5=$("$MD5SUM" "$output_file" | awk '{print $1}')
        if [[ "$INPUT_MD5" != "$output_md5" ]]; then
            echo "MD5 mismatch. Logs retained at: $work_dir" >&2
            echo "Input:  $INPUT_MD5" >&2
            echo "Output: $output_md5" >&2
            exit 1
        fi

        if (( round_index < WARMUPS )); then
            echo "Warm-up completed: ${real_seconds}s; MD5 OK"
        else
            run_number=$((round_index - WARMUPS + 1))
            printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
                "$mechanism" "$case_name" "$buffer_size" \
                "$effective_chunk_size" "$FILE_SIZE_BYTES" \
                "$run_number" "$real_seconds" "$user_seconds" "$system_seconds" \
                "$INPUT_MD5" "$output_md5" >> "$CSV_FILE"
            echo "run=$run_number, real=${real_seconds}s, user=${user_seconds}s, system=${system_seconds}s; MD5 OK"
        fi

        rm -rf -- "$work_dir"
    done
done

echo
echo "Measurements saved to: $CSV_FILE"
"$PYTHON" "$SCRIPT_DIR/run_benchmarks.py" --csv "$CSV_FILE" --output-dir "$OUTPUT_DIR"
echo "Benchmark completed."

