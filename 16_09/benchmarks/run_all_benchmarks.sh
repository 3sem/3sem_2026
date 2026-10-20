#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 9 ]]; then
    echo "Usage: $0 <shm_sender> <shm_receiver> <queue_sender> <queue_receiver> <fifo_sender> <fifo_receiver> <results_dir> <python> <md5sum>" >&2
    exit 2
fi

SHM_SENDER=$1
SHM_RECEIVER=$2
QUEUE_SENDER=$3
QUEUE_RECEIVER=$4
FIFO_SENDER=$5
FIFO_RECEIVER=$6
RESULTS_DIR=$7
PYTHON=$8
MD5SUM=$9

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
BENCHMARK_SCRIPT="$SCRIPT_DIR/run_benchmark.sh"

for item in \
    "512MiB:536870912" \
    "1GiB:1073741824" \
    "2GiB:2147483648"
do
    IFS=: read -r name size <<< "$item"

    echo
    echo "========================================"
    echo "Running benchmark: $name"
    echo "Results directory: $RESULTS_DIR/$name"
    echo "========================================"

    FILE_SIZE_BYTES="$size" \
    REPEATS="${REPEATS:-5}" \
    WARMUPS="${WARMUPS:-1}" \
    bash "$BENCHMARK_SCRIPT" \
        "$SHM_SENDER" \
        "$SHM_RECEIVER" \
        "$QUEUE_SENDER" \
        "$QUEUE_RECEIVER" \
        "$FIFO_SENDER" \
        "$FIFO_RECEIVER" \
        "$RESULTS_DIR/$name" \
        "$PYTHON" \
        "$MD5SUM"
done

echo
echo "All benchmark runs completed."
echo "Results saved under: $RESULTS_DIR"
