CC = gcc
CFLAGS = -Wall -Wextra -O3
LIBS = -pthread

all: progA progB

progA: progA.c common.h
	$(CC) $(CFLAGS) progA.c -o progA $(LIBS)

progB: progB.c common.h
	$(CC) $(CFLAGS) progB.c -o progB

clean:
	rm -f progA progB