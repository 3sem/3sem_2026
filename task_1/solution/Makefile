CXX := g++
CXXFLAGS := -Wall -Wextra -g -fsanitize=address,undefined -fno-omit-frame-pointer
TARGET := firstTask
COMMON_DIR := ../../common
CPPFLAGS += -I$(COMMON_DIR)
SOURCES := main.cpp command.cpp parser.cpp general.cpp $(COMMON_DIR)/linuxCmd.cpp

.PHONY: all clean

all:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
