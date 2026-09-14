#!/usr/bin/env bash

./genTestFile.sh
./fullDuplexPipe testFile testFile.out
md5sum testFile testFile.out
rm testFile testFile.out
