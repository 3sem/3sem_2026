CC = cc
CPPFLAGS += -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -Iheaders
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic
PYTHON ?= python3

PROGRAMS = shm_transfer mq_transfer fifo_transfer ipc_info
COMMON = build/main.o build/io.o build/probe.o
OBJECTS = $(COMMON) build/shm.o build/mq.o build/fifo.o build/ipc_info.o

.PHONY: all clean test benchmark
all: $(PROGRAMS)

shm_transfer: $(COMMON) build/shm.o
mq_transfer: $(COMMON) build/mq.o
fifo_transfer: $(COMMON) build/fifo.o
ipc_info: build/ipc_info.o build/probe.o

$(PROGRAMS):
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: sources/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p $@

test: all
	$(PYTHON) scripts/test_transfer.py

benchmark: all
	$(PYTHON) scripts/benchmark.py

clean:
	$(RM) -r build $(PROGRAMS)

-include $(OBJECTS:.o=.d)
