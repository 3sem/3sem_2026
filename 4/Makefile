CC = gcc

DIRSOURCE       = Source
CFLAGSH         = -IInclude -D FIFO
# SHM запуск разделяемой памяти
# SYS_V_QUEUE запуск очереди сообщений
# FIFO запуск FIFO

#sudo sysctl -w kernel.msgmax=67108864
#sudo sysctl -w kernel.msgmnb=134217728
#sudo sysctl -w kernel.msgmni=32000

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

start_time_test: 
	rm -f *.txt
	touch out.txt

	@for i in $$(seq 1 1 20); do \
		./$(DIR_BUILD)/$(EXECUTABLE_NAME) 2>&1 | grep sec >> out.txt; \
	done


start:
	./$(DIR_BUILD)/$(EXECUTABLE_NAME)

clean:
	rm -rf ./$(DIR_BUILD)/
	rm *.txt

check:
	@md5sum Destination.txt 
	@md5sum Source.txt 
