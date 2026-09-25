#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
    echo "Usage: $0 <fifo|shMem|queue> <--send|--read> [size_in_bytes]" >&2
    exit 1
fi

case "$1" in
    fifo|FIFO) transfer_type=fifo ;;
    shMem|shmem|SHMEM) transfer_type=shMem ;;
    queue|QUEUE) transfer_type=queue ;;
    *)
        echo "Unknown transfer type: $1. Expected fifo, shMem or queue." >&2
        exit 1
        ;;
esac

case "$2" in
    --send|-s|send) mode=send ;;
    --read|-r|read) mode=read ;;
    *)
        echo "Unknown mode: $2. Expected --send or --read." >&2
        exit 1
        ;;
esac

file_size=${3-1048576}
if [[ ! $file_size =~ ^[0-9]+$ ]]; then
    echo "File size must be a non-negative integer in bytes." >&2
    exit 1
fi

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
source ./transfer.conf

if [[ $transfer_type == shMem ]]; then
    touch -- "$SHMEM_PATH"
elif [[ $transfer_type == queue ]]; then
    touch -- "$QUEUE_PATH"
elif [[ $mode == read ]]; then
    rm -f -- "$FIFO_PATH"
fi

if [[ $mode == send ]]; then
    bash ./genTestFile.sh testFile "$file_size"
fi
