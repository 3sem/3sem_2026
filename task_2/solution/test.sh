#!/usr/bin/env bash

dd if=/dev/urandom of=testFile bs=1048576 count=4096
./fullDuplexPipe testFile testFile.out
md5sum testFile testFile.out
rm testFile testFile.out
