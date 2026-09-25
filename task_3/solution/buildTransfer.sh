#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <fifo|shMem|queue>" >&2
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

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

cmake -S . -B build "${cmake_options[@]}"
cmake --build build --target ipcTransfer
