CC = cc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic

.PHONY: all run clean

all: shell

shell: shell.c
	$(CC) $(CFLAGS) $< -o $@

run: shell
	./shell

clean:
	rm -f shell
