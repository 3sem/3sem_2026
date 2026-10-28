CC       = gcc
CFLAGS   = -O3 -Wall -Wextra -std=c11 -pthread
LIBS     = -lrt -lpthread

DIR_NOSYNC = no_sync
DIR_SYNC   = with_sync

TARGETS = $(DIR_NOSYNC)/program_A \
          $(DIR_NOSYNC)/program_B \
          $(DIR_SYNC)/program_A \
          $(DIR_SYNC)/program_B

all: $(TARGETS)
	@echo "Сборка проекта на Си завершена успешно."

$(DIR_NOSYNC)/program_A: $(DIR_NOSYNC)/program_A.c $(DIR_NOSYNC)/shared_data.h
	$(CC) $(CFLAGS) $(DIR_NOSYNC)/program_A.c -o $(DIR_NOSYNC)/program_A $(LIBS)

$(DIR_NOSYNC)/program_B: $(DIR_NOSYNC)/program_B.c $(DIR_NOSYNC)/shared_data.h
	$(CC) $(CFLAGS) $(DIR_NOSYNC)/program_B.c -o $(DIR_NOSYNC)/program_B $(LIBS)

$(DIR_SYNC)/program_A: $(DIR_SYNC)/program_A.c $(DIR_SYNC)/shared_data.h
	$(CC) $(CFLAGS) $(DIR_SYNC)/program_A.c -o $(DIR_SYNC)/program_A $(LIBS)

$(DIR_SYNC)/program_B: $(DIR_SYNC)/program_B.c $(DIR_SYNC)/shared_data.h
	$(CC) $(CFLAGS) $(DIR_SYNC)/program_B.c -o $(DIR_SYNC)/program_B $(LIBS)


#Тесты без синхронизации (N увеличено до 500 000 000)
test_nosync: $(TARGETS)
	@killall -9 program_B program_A 2>/dev/null || true
	@echo "=== ТЕСТЫ БЕЗ СИНХРОНИЗАЦИИ (N = 500M, CPU Affinity) ==="
	@for n in 1 2 4 8 16 32; do \
		sleep 15 | ./$(DIR_NOSYNC)/program_B 500000000 $$n & PID_B=$$!; \
		sleep 0.4; \
		echo -n "Потоков $$n: Время = "; \
		./$(DIR_NOSYNC)/program_A 500000000 $$n; \
		kill -9 $$PID_B 2>/dev/null; \
		wait $$PID_B 2>/dev/null; \
	done

#Тесты с синхронизацией (оказались не нужными)
test_sync: $(TARGETS)
	@killall -9 program_B program_A 2>/dev/null || true
	@echo "=== ТЕСТЫ С СИНХРОНИЗАЦИЕЙ (N = 10M) ==="
	@for n in 1 2 4 8 16; do \
		sleep 25 | ./$(DIR_SYNC)/program_B 10000000 $$n & PID_B=$$!; \
		sleep 0.4; \
		echo -n "Потоков $$n: Время = "; \
		./$(DIR_SYNC)/program_A 10000000 $$n; \
		kill -9 $$PID_B 2>/dev/null; \
		wait $$PID_B 2>/dev/null; \
	done

clean:
	rm -f $(TARGETS)
	rm -f /dev/shm/mc_shm_nosync /dev/shm/mc_shm_sync

.PHONY: all clean test_nosync test_sync
