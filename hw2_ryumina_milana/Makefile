CC      = gcc
TARGET  = duplex_pipe
SRCS    = main.c duplex_channel.c file.c

BUILD   = build
OBJS    = $(addprefix $(BUILD)/, $(SRCS:.c=.o))

DEBUG_FLAGS   = -Wall -Wextra -g -O0 -D_DEBUG
RELEASE_FLAGS = -Wall -Wextra -O2

all: release

debug: CFLAGS  = $(DEBUG_FLAGS)
debug: LDFLAGS = $(DEBUG_FLAGS)
debug: $(BUILD)/$(TARGET)

release: CFLAGS  = $(RELEASE_FLAGS)
release: LDFLAGS = $(RELEASE_FLAGS)
release: $(BUILD)/$(TARGET)

$(BUILD)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILD)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

.PHONY: all debug release clean