CC = cc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -D_FILE_OFFSET_BITS=64
LDLIBS = -pthread -lrt -lm
THREADS ?= 4
POINTS ?= 100000000
REPEATS ?= 3

.PHONY: all run benchmark clean

all: program_a program_b

program_a: program_a.c shared.h
	$(CC) $(CFLAGS) program_a.c -o $@ $(LDLIBS)

program_b: program_b.c shared.h
	$(CC) $(CFLAGS) program_b.c -o $@ $(LDLIBS)

run: all
	./program_a $(THREADS) $(POINTS)
	./program_b

benchmark: all
	python3 benchmark.py --points $(POINTS) --repeats $(REPEATS)

clean:
	rm -f program_a program_b
