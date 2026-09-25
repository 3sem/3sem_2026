#!/usr/bin/env bash
set -euo pipefail

test_file="testFile"
test_file_out="testFileOut"

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <fifo|shMem|queue> <--send|--read> [file_size] [--chunk-size BYTES] [--no-prepare] [--no-check]" >&2
    echo "Size is used when sending; default: 1048576 bytes." >&2
    echo "Start the reader first for fifo, or the sender first for shMem." >&2
    exit 1
fi

file_size=1048576
chunk_size=4096
prepare=1
check=1
size_given=0

transfer_argument=$1
mode_argument=$2
shift 2

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-prepare) prepare=0 ;;
        --no-check) check=0 ;;
        --chunk-size|-b)
            if [[ $# -lt 2 || ! $2 =~ ^[0-9]+$ || ! $2 =~ [1-9] ]]; then
                echo "Chunk size must be a positive integer in bytes." >&2
                exit 1
            fi
            chunk_size=$2
            shift
            ;;
        *)
            if [[ $size_given == 1 || ! $1 =~ ^[0-9]+$ ]]; then
                echo "Invalid size or option: $1" >&2
                exit 1
            fi
            file_size=$1
            size_given=1
            ;;
    esac
    shift
done

if [[ ! $file_size =~ ^[0-9]+$ ]]; then
    echo "File size must be a non-negative integer in bytes." >&2
    exit 1
fi

case "$transfer_argument" in
    fifo|FIFO)
        transfer_type=fifo
        ;;
    shMem|shmem|SHMEM)
        transfer_type=shMem
        ;;
    queue|QUEUE)
        transfer_type=queue
        ;;
    *)
        echo "Unknown transfer type: $transfer_argument. Expected fifo, shMem or queue." >&2
        exit 1
        ;;
esac

case "$mode_argument" in
    --send|-s|send)
        mode=send
        ;;
    --read|-r|read)
        mode=read
        ;;
    *)
        echo "Unknown mode: $mode_argument. Expected --send or --read." >&2
        exit 1
        ;;
esac

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

if [[ ! -x ./build/ipcTransfer ]]; then
    echo "Build the selected transfer type first: bash ./buildTransfer.sh $transfer_type" >&2
    exit 1
fi

if [[ $prepare == 1 ]]; then
    bash ./prepareTransfer.sh "$transfer_type" "$mode" "$file_size"
fi

if [[ $mode == send ]]; then
    echo "Sending $test_file using $transfer_type, chunk size $chunk_size..."
    exec ./build/ipcTransfer -f "$test_file" -s -b "$chunk_size"
else
    echo "Receiving $test_file_out using $transfer_type, chunk size $chunk_size..."
    if [[ $check == 0 ]]; then
        exec ./build/ipcTransfer -f "$test_file_out" -r -b "$chunk_size"
    fi
    ./build/ipcTransfer -f "$test_file_out" -r -b "$chunk_size"
    input_checksum=$(md5sum < "$test_file")
    output_checksum=$(md5sum < "$test_file_out")

    if [[ ${input_checksum%% *} == "${output_checksum%% *}" ]]; then
        echo "MD5 checksums match. File transferred successfully."
    else
        echo "MD5 checksums do not match."
    fi

    rm -f -- "$test_file" "$test_file_out"
fi
