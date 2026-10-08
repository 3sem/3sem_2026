[ -f input.bin ] || dd if=/dev/urandom of=input.bin bs=1048576 count=4096
clang -O3 2_task.c
time ./a.out input.bin output.bin
md5 input.bin output.bin
