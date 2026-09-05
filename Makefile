CXX := g++
CXXFLAGS := -Wall -Wextra
TARGET := firstTask
SOURCES := main.cpp command.cpp parser.cpp general.cpp

.PHONY: all clean

all:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
