#!/usr/bin/env bash

bash ./genTestFile.sh
"${1:-./build/debug/fullDuplexPipe}" testFile testFile.out
md5sum testFile testFile.out
rm testFile testFile.out
