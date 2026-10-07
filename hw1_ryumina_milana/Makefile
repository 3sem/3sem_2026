CC      = gcc
TARGET  = cmd_emulator
SRCS    = main.c command_parser.c read_cmd.c run_cmd.c

DEBUG_FLAGS   = -Wall -Wextra -g -O0 -D_DEBUG

RELEASE_FLAGS = -Wall -Wextra -O2

all: release

debug:
	$(CC) $(DEBUG_FLAGS) -o $(TARGET) $(SRCS)

release:
	$(CC) $(RELEASE_FLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
