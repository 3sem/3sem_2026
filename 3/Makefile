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
	#dd if=/dev/urandom of=Source.txt bs=1048576 count=4096
	mkdir -p $(DIR_BUILD)/$(DIRSOURCE)/

# результаты будут в out.txt
start_time_test: 
	touch out.txt
	rm -f *.txt
	dd if=/dev/urandom of=Source.txt bs=1048576 count=4096

	@for i in $$(seq 1 1 5); do \
		rm -f Destination.txt; \
		sync; \
		sleep 1; \
		./$(DIR_BUILD)/$(EXECUTABLE_NAME) 2>&1 | grep sec >> out.txt; \
	done


start:
	rm -f Destination.txt
	./$(DIR_BUILD)/$(EXECUTABLE_NAME)

clean:
	rm -rf ./$(DIR_BUILD)/
	rm *.txt

check:
	@md5sum Destination.txt 
	@md5sum Source.txt 
