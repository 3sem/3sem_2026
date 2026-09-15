CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -Iheaders

TARGET = terminal

SRC = terminal.c \
      sources/terminal_funcs.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC) headers/terminal_funcs.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
