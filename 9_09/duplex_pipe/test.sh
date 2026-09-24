#!/bin/bash

set -e

DIR="txt"
INPUT="$DIR/input.txt"
OUTPUT="$DIR/output.txt"

mkdir -p "$DIR"

echo "=== Build ==="
make

echo "=== Generate 4 GB file ==="
dd if=/dev/urandom of="$INPUT" bs=1M count=4096 status=progress

echo "=== Run duplex pipe ==="
./build/out

echo "=== Check MD5 ==="
md5sum "$INPUT"
md5sum "$OUTPUT"

echo "=== Compare ==="
if cmp -s "$INPUT" "$OUTPUT"; then
    echo "TEST PASSED"
else
    echo "TEST FAILED"
    exit 1
fi

echo "=== Cleanup ==="
rm -f "$INPUT" "$OUTPUT"
rm -rf "$DIR"

echo "Done."
