CC      = gcc
BUILD   = build

SHM_BIN   = shared_mem
MQ_BIN    = message_queue
FIFO_BIN  = named_pipe

SHM_SRC   = shared_mem.c
MQ_SRC    = message_queue.c
FIFO_SRC  = named_pipe.c

# Общие флаги: предупреждения и пути
COMMON_FLAGS = -Wall -Wextra -I.

# ---- Debug: отладка ----
DEBUG_FLAGS   = $(COMMON_FLAGS) -g -O0 -D_DEBUG

# ---- Release: финальная ----
RELEASE_FLAGS = $(COMMON_FLAGS) -O2

# ---- Sanitize: debug + проверки памяти ----
SANITIZE_FLAGS = $(DEBUG_FLAGS) -fsanitize=address,undefined,leak

.PHONY: all debug release sanitize clean

all: release

debug: CFLAGS  = $(DEBUG_FLAGS)
debug: LDFLAGS = $(DEBUG_FLAGS)
debug: $(BUILD)/$(SHM_BIN) $(BUILD)/$(MQ_BIN) $(BUILD)/$(FIFO_BIN)

release: CFLAGS  = $(RELEASE_FLAGS)
release: LDFLAGS = $(RELEASE_FLAGS)
release: $(BUILD)/$(SHM_BIN) $(BUILD)/$(MQ_BIN) $(BUILD)/$(FIFO_BIN)

sanitize: CFLAGS  = $(SANITIZE_FLAGS)
sanitize: LDFLAGS = $(SANITIZE_FLAGS)
sanitize: $(BUILD)/$(SHM_BIN) $(BUILD)/$(MQ_BIN) $(BUILD)/$(FIFO_BIN)

# ---- Сборка отдельных бинарников ----
$(BUILD)/$(SHM_BIN): $(SHM_SRC) shared_mem.h common.h | $(BUILD)
	$(CC) $(CFLAGS) $(SHM_SRC) -o $@ $(LDFLAGS)

$(BUILD)/$(MQ_BIN): $(MQ_SRC) common.h | $(BUILD)
	$(CC) $(CFLAGS) $(MQ_SRC) -o $@ $(LDFLAGS)

$(BUILD)/$(FIFO_BIN): $(FIFO_SRC) common.h | $(BUILD)
	$(CC) $(CFLAGS) $(FIFO_SRC) -o $@ $(LDFLAGS)

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
	@echo "Cleaned."