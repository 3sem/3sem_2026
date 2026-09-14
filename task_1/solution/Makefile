CC = gcc
CPPFLAGS += -I../../common -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64
CFLAGS ?= -Wall -Wextra -Wpedantic
MODE ?= debug

DEBUG_FLAGS := -O0 -g3
RELEASE_FLAGS := -O2

ifeq ($(MODE),debug)
MODE_FLAGS := $(DEBUG_FLAGS)
else ifeq ($(MODE),release)
MODE_FLAGS := $(RELEASE_FLAGS)
else
$(error MODE must be debug or release)
endif

TARGET := firstTask
BUILD_DIR := build/$(MODE)
BINARY := $(BUILD_DIR)/$(TARGET)
SOURCES := main.c command.c parser.c general.c
OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o) $(BUILD_DIR)/linuxCmd.o
DEPS := $(OBJECTS:.o=.d)

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.PHONY: all debug release clean rebuild run

all: $(BINARY)

debug release:
	$(MAKE) all MODE=$@

$(BINARY): $(OBJECTS)
	$(CC) $(CFLAGS) $(MODE_FLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MODE_FLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/linuxCmd.o: ../../common/linuxCmd.c Makefile | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MODE_FLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

run: all
	./$(BINARY) $(ARGS)

clean:
	$(RM) $(foreach mode,debug release,$(addprefix build/$(mode)/,$(notdir $(OBJECTS) $(DEPS)) $(TARGET)))

rebuild:
	$(MAKE) clean
	$(MAKE) all MODE=$(MODE)

-include $(DEPS)
