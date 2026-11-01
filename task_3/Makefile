CC := gcc
CFLAGS := -std=c11 -D_GNU_SOURCE -Wall -Wextra -O2

# В среде Cursor драйвер gcc теряет свой префикс и не находит cc1.
# Явный -B на каталог нужной версии безвреден и на обычной системе.
GCC_MACHINE := $(shell $(CC) -dumpmachine)
GCC_VERSION := $(shell $(CC) -dumpversion)
GCC_MAJOR := $(firstword $(subst ., ,$(GCC_VERSION)))
CC1_DIR := $(firstword \
	$(wildcard /usr/lib/gcc/$(GCC_MACHINE)/$(GCC_VERSION)) \
	$(wildcard /usr/lib/gcc/$(GCC_MACHINE)/$(GCC_MAJOR)))
ifneq ($(CC1_DIR),)
CFLAGS += -B$(CC1_DIR)/
endif

BINDIR := bin
BINS := shm_sender shm_receiver mq_sender mq_receiver fifo_sender fifo_receiver

.PHONY: all clean bench smoke

all: $(addprefix $(BINDIR)/,$(BINS))

$(BINDIR)/%: src/%.c src/common.c src/common.h
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ src/$*.c src/common.c

bench: all
	./bench.sh

smoke: all
	./bench.sh smoke

clean:
	rm -rf $(BINDIR)
