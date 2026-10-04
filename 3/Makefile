CC = gcc

DIRSOURCE       = Source
CFLAGSH         = -IInclude -DSYS_V_QUEUE
# SYS_V_QUEUE запуск очереди сообщений
# SHM запуск разделяемой памяти
# FIFO запуск FIFO

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

# результаты будут в out.txt
start_time_test: 
	touch out.txt
	sudo cpupower frequency-set -g performance > /dev/null
	dd if=/dev/urandom of=Source.txt bs=1048576 count=64


	sleep 1

	@for i in 1 2 3 4 5 6 7; do \
		rm -f Destination.txt; \
		sync; \
		sleep 1; \
		cat Source.txt > /dev/null; \
		./$(DIR_BUILD)/$(EXECUTABLE_NAME) 2>&1 | grep sec >> out.txt; \
	done

	sudo cpupower frequency-set -g powersave > /dev/null

start:
	./$(DIR_BUILD)/$(EXECUTABLE_NAME)

clean:
	rm -rf ./$(DIR_BUILD)/
	rm *.txt

check:
	@md5sum Destination.txt 
	@md5sum Source.txt 
