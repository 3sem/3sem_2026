CC      = gcc
CFLAGS  = -Wall -Wextra -O2
PYTHON  = python3

BINS = shm_ipc msg_ipc fifo_ipc

FILE_IN   = input.dat
FILE_OUT  = output.dat

BUF_4K  = 4096
BUF_64K = 65536
BUF_1M  = 1048576

.PHONY: all clean test plot

all: $(BINS)

shm_ipc: shm_ipc.c
	$(CC) $(CFLAGS) $< -o $@

msg_ipc: msg_ipc.c
	$(CC) $(CFLAGS) $< -o $@

fifo_ipc: fifo_ipc.c
	$(CC) $(CFLAGS) $< -o $@

$(FILE_IN):
	@echo "==> Генерация тестового файла 100 МБ..."
	dd if=/dev/urandom of=$(FILE_IN) bs=1M count=100 status=none

test: all $(FILE_IN)
	@echo "==> Исходный MD5: $$(md5sum $(FILE_IN) | cut -d' ' -f1)"
	@echo "==> Запуск тестов..."
	
	@echo "\n--- Буфер: 4 КБ ---"
	./shm_ipc $(FILE_IN) $(FILE_OUT) $(BUF_4K)
	@md5sum $(FILE_OUT)
	./msg_ipc $(FILE_IN) $(FILE_OUT) $(BUF_4K)
	./fifo_ipc $(FILE_IN) $(FILE_OUT) $(BUF_4K)
	
	@echo "\n--- Буфер: 64 КБ ---"
	./shm_ipc $(FILE_IN) $(FILE_OUT) $(BUF_64K)
	./msg_ipc $(FILE_IN) $(FILE_OUT) $(BUF_64K)
	./fifo_ipc $(FILE_IN) $(FILE_OUT) $(BUF_64K)
	
	@echo "\n--- Буфер: 1 МБ ---"
	./shm_ipc $(FILE_IN) $(FILE_OUT) $(BUF_1M)
	./msg_ipc $(FILE_IN) $(FILE_OUT) $(BUF_1M)
	./fifo_ipc $(FILE_IN) $(FILE_OUT) $(BUF_1M)
	
	@rm -f $(FILE_OUT)
	@echo "\n✅ Все тесты завершены."

 
plot:
	$(PYTHON) graph.py

clean:
	rm -f $(BINS) $(FILE_IN) $(FILE_OUT)
