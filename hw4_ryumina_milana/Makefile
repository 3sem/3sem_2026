CC     = gcc
BUILD  = build

COMMON_FLAGS = -Wall -Wextra -I.

DEBUG_FLAGS   = $(COMMON_FLAGS) -g -O0 -D_DEBUG
RELEASE_FLAGS = $(COMMON_FLAGS) -O2

LIBS = -lm -lpthread

.PHONY: all debug release clean

all: release

debug: CFLAGS = $(DEBUG_FLAGS)
debug: $(BUILD)/worker $(BUILD)/reader

release: CFLAGS = $(RELEASE_FLAGS)
release: $(BUILD)/worker $(BUILD)/reader

$(BUILD)/worker: worker.c main.c monte_carlo.h | $(BUILD)
	$(CC) $(CFLAGS) worker.c main.c -o $@ $(LIBS)

$(BUILD)/reader: reader.c monte_carlo.h | $(BUILD)
	$(CC) $(CFLAGS) reader.c -o $@ $(LIBS)

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
	@echo "Cleaned."