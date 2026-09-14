# GCC 14+ (C23)
CC = gcc
CPPFLAGS += -I../../common -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64
CFLAGS ?= -std=c23 -Wall -Wextra -Wpedantic -O2 -g

TARGET := firstTask
BUILD_DIR := build
SOURCES := main.c command.c parser.c general.c
OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o) $(BUILD_DIR)/linuxCmd.o
DEPS := $(OBJECTS:.o=.d)

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.PHONY: all clean rebuild run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/linuxCmd.o: ../../common/linuxCmd.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

run: all
	./$(TARGET) $(ARGS)

clean:
	$(RM) $(OBJECTS) $(DEPS) $(TARGET)

rebuild:
	$(MAKE) clean
	$(MAKE) all

-include $(DEPS)
