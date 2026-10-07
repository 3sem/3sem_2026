#!/bin/bash
if [ ! -f parent.txt ]; then
    echo "Generating test file..."
    dd if=/dev/urandom of=parent.txt bs=1048576 count=4096 status=none
status=none
fi

for size in 64 128 256 512 1024; do
    echo "=== Buffer: ${size} KB ==="
    rm -f child.txt  # удалить старый
    ./build/duplex_pipe $size

    parent_md5=$(md5sum parent.txt | cut -d' ' -f1)
    child_md5=$(md5sum child.txt | cut -d' ' -f1)

    if [ "$parent_md5" = "$child_md5" ]; then
        echo "OK"
    else
        echo "FAIL"
        echo "Parent: $parent_md5" 
        echo "Child:  $child_md5"
        exit 1
    fi
done



