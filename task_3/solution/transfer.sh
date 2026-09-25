#!/usr/bin/env bash
set -euo pipefail

test_file="testFile"
test_file_out="testFileOut"

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <fifo|shMem|queue> <--send|--read>" >&2
    echo "Start the reader first for fifo, or the sender first for shMem." >&2
    exit 1
fi

case "$1" in
    fifo|FIFO)
        cmake_options=(-DFIFO=ON -DSHMEM=OFF -DQUEUE=OFF)
        ;;
    shMem|shmem|SHMEM)
        cmake_options=(-DFIFO=OFF -DSHMEM=ON -DQUEUE=OFF)
        ;;
    queue|QUEUE)
        cmake_options=(-DFIFO=OFF -DSHMEM=OFF -DQUEUE=ON)
        ;;
    *)
        echo "Unknown transfer type: $1. Expected fifo, shMem or queue." >&2
        exit 1
        ;;
esac

case "$2" in
    --send|-s|send)
        mode=send
        ;;
    --read|-r|read)
        mode=read
        ;;
    *)
        echo "Unknown mode: $2. Expected --send or --read." >&2
        exit 1
        ;;
esac

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

cmake -S . -B build "${cmake_options[@]}"
cmake --build build --target ipcTransfer

source ./transfer.conf

if [[ ${cmake_options[1]} == -DSHMEM=ON ]]; then
    touch -- "$SHMEM_PATH"
elif [[ ${cmake_options[2]} == -DQUEUE=ON ]]; then
    touch -- "$QUEUE_PATH"
elif [[ $mode == read ]]; then
    rm -f -- "$FIFO_PATH"
fi

if [[ $mode == send ]]; then
    bash ./genTestFile.sh "$test_file"
    echo "Sending $test_file using $1..."
    ./build/ipcTransfer -f "$test_file" -s
else
    echo "Receiving $test_file_out using $1..."
    ./build/ipcTransfer -f "$test_file_out" -r
    input_checksum=$(md5sum < "$test_file")
    output_checksum=$(md5sum < "$test_file_out")

    if [[ ${input_checksum%% *} == "${output_checksum%% *}" ]]; then
        echo "MD5 checksums match. File transferred successfully."
    else
        echo "MD5 checksums do not match."
    fi

    rm -f -- "$test_file" "$test_file_out"
fi
