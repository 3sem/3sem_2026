#!/usr/bin/env bash
set -euo pipefail

if [[ $# -gt 2 ]]; then
    echo "Usage: $0 [file_name] [size_in_bytes]" >&2
    exit 1
fi

test_file=${1:-testFile}
file_size=${2-1048576}

if [[ ! $file_size =~ ^[0-9]+$ ]]; then
    echo "File size must be a non-negative integer in bytes." >&2
    exit 1
fi

dd if=/dev/urandom of="$test_file" bs=1048576 count="$file_size" iflag=count_bytes,fullblock
