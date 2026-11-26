CC = gcc

DIRSOURCE       = Source
CFLAGSH         = -IInclude
SOURCES         = $(wildcard $(DIRSOURCE)/*.c)
OBJECTS         = $(patsubst %.c, %.o, $(SOURCES))

EXECUTABLE_NAME = main.exe
DIR_BUILD       = Build

all: $(EXECUTABLE_NAME)

.PHONY = all clean start

$(EXECUTABLE_NAME): make_folder  $(OBJECTS)
	$(CC) $(addprefix ./$(DIR_BUILD)/, $(OBJECTS)) -o ./$(DIR_BUILD)/$(EXECUTABLE_NAME)

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGSH) -c $^ -o ./$(DIR_BUILD)/$@

make_folder:
	mkdir -p $(DIR_BUILD)/$(DIRSOURCE)/

start:
	./$(DIR_BUILD)/$(EXECUTABLE_NAME)


start_val:
	valgrind ./$(DIR_BUILD)/$(EXECUTABLE_NAME) -Wall -Wallstring

clean:
	rm -rf ./$(DIR_BUILD)/