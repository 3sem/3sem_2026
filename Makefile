# Сборка:   make
# Проверка: make check          (файл 8 МиБ, оба режима, сравнение байт)
# Запуск:   ./echo_test входной_файл выходной_файл
#           ./echo_test --copy входной_файл выходной_файл

CC = gcc
CFLAGS = -O2 -Wall -Wextra -D_FILE_OFFSET_BITS=64

.PHONY: all check clean

all: echo_test

echo_test: main.c echo.c duplex_pipe.c echo.h duplex_pipe.h
	$(CC) $(CFLAGS) -o echo_test main.c echo.c duplex_pipe.c

check: echo_test
	dd if=/dev/urandom of=/tmp/fdp_in bs=1048576 count=8 status=none
	timeout 60 ./echo_test /tmp/fdp_in /tmp/fdp_out
	cmp /tmp/fdp_in /tmp/fdp_out
	timeout 60 ./echo_test --copy /tmp/fdp_in /tmp/fdp_out
	cmp /tmp/fdp_in /tmp/fdp_out
	rm -f /tmp/fdp_in /tmp/fdp_out
	@echo CHECK_OK

clean:
	rm -f echo_test
