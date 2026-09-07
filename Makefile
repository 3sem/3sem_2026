CXX := g++
CXXFLAGS := -Wall -Wextra -g -fsanitize=address,undefined -fno-omit-frame-pointer
TARGET := firstTask
SOURCES := main.cpp command.cpp parser.cpp general.cpp linuxCmd.cpp

.PHONY: all clean

all:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
