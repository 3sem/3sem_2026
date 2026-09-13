CXX := g++
CXXFLAGS := -Wall -Wextra -g -std=c++11 -pthread
TARGET := fullDuplexPipe
COMMON_DIR := ../../common
CPPFLAGS += -I$(COMMON_DIR)
CPPFLAGS += -D_FILE_OFFSET_BITS=64
SOURCES := main.cpp echoTest.cpp fullDuplexPipe.cpp fileBuffer.cpp $(COMMON_DIR)/linuxCmd.cpp
HEADERS := echoTest.h fullDuplexPipe.h fileBuffer.h $(COMMON_DIR)/linuxCmd.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
