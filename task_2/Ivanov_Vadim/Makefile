CC = cc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic
INPUT = input.bin
OUTPUT = output.bin
CHUNK = 1048576

.PHONY: all run clean

all: duplex_echo

duplex_echo: echo.c duplex.c duplex.h
	$(CC) $(CFLAGS) echo.c duplex.c -pthread -o $@

run: duplex_echo
	./duplex_echo "$(INPUT)" "$(OUTPUT)" "$(CHUNK)"
	md5sum "$(INPUT)" "$(OUTPUT)"
	cmp "$(INPUT)" "$(OUTPUT)"

clean:
	rm -f duplex_echo
