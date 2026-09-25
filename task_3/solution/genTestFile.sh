#!/usr/bin/env bash

test_file=${1:-testFile}
dd if=/dev/urandom of="$test_file" bs=1048576 count=1
