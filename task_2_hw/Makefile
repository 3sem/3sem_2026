CC = cc
CPPFLAGS += -Iheaders -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic

TARGET = duplex_echo
OBJECTS = build/main.o build/full_duplex_pipe.o

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: sources/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p $@

clean:
	rm -rf build $(TARGET)

-include $(OBJECTS:.o=.d)
