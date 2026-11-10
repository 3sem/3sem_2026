CC      = gcc
CFLAGS  = -Wall -Wextra -g -I.
LDFLAGS =
TARGET  = terminal

OBJS = main.o tok.o execute.o

HEADERS = tok.h execute.h


all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^


main.o: main.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

tok.o: tok.c tok.h
	$(CC) $(CFLAGS) -c $< -o $@

execute.o: execute.c execute.h
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJS) $(TARGET)


rebuild: clean all


run: all
	./$(TARGET)

.PHONY: all clean rebuild run