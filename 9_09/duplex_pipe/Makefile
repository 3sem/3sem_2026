ifeq ($(origin CC), default) 
	CC = gcc
endif

CFLAGS ?= -Wall -Wextra -Wpedantic -g 
OUT_O_DIR ?= build
COMMONINC = -I./include
SRC = src
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

override CFLAGS += $(COMMONINC)

CSRC = src/main.c src/dup_pipe.c

COBJ := $(addprefix $(OUT_O_DIR)/,$(CSRC:.c=.o)) 

DEPS = $(COBJ:.o=.d)

.PHONY: all
all: $(OUT_O_DIR)/out

$(OUT_O_DIR)/out : $(COBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(COBJ) : $(OUT_O_DIR)/%.o : %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(DEPS) : $(OUT_O_DIR)/%.d : %.c
	@mkdir -p $(@D)
	$(CC) -E $(CFLAGS) $< -MM -MT $(@:.d=.o) > $@


PHONY: clean
clean:
	rm -rf $(OUT_O_DIR)
