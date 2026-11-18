CC = gcc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -D_FILE_OFFSET_BITS=64
LDLIBS = -pthread

PROGRAMS = shm_transfer mq_transfer fifo_transfer

all: $(PROGRAMS)

shm_transfer: shm.c common.c common.h
	$(CC) $(CFLAGS) shm.c common.c -o $@ $(LDLIBS)

mq_transfer: mq.c common.c common.h
	$(CC) $(CFLAGS) mq.c common.c -o $@ $(LDLIBS)

fifo_transfer: fifo.c common.c common.h
	$(CC) $(CFLAGS) fifo.c common.c -o $@ $(LDLIBS)

bench: all
	python3 benchmark.py "$(FILE)"

clean:
	rm -f $(PROGRAMS)

.PHONY: all bench clean
