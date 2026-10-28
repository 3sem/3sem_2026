## Сборка:
```
# Разделяемая память SYS V:
gcc -O2 -Wall -Wextra -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 main.c common.c sharedMemory.c -o shm_transfer

# Очередь сообщений SYS V:
gcc -O2 -Wall -Wextra -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 main.c common.c messageQueue.c -o mq_transfer

# Именованные каналы (FIFO):
gcc -O2 -Wall -Wextra -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 main.c common.c fifo.c -o fifo_transfer
```

## Запуск тестов:

### Создание передаваемого файла:
```
dd if=/dev/urandom of=<file_name> bs=1048576 count=4096
```

### Увеличение ограничений очередей сообщений:

```
sudo sysctl -w kernel.msgmax=1048576
sudo sysctl -w kernel.msgmnb=1048576
```

### Запуск скрипта:
```
./test.sh <file_name>
```

## Результаты и выводы

Наименьшее среднее время показала очередь сообщений SYS V при размере сообщения **1 МиБ — 2,425 с**. При буфере 8 КиБ быстрее также оказалась очередь сообщений SYS V.

Для разделяемой памяти увеличение буфера с 8 КиБ до 1 МиБ сократило время передачи примерно на **62%**. Вероятная причина — уменьшение количества передаваемых блоков и операций с семафорами.

Для очереди сообщений переход с 8 до 64 КиБ уменьшил время примерно на **28%**, а при 1 МиБ среднее время снизилось до **2,425 с**.

Для FIFO лучший результат получен при буфере **64 КиБ — 3,343 с**. При 1 МиБ время выросло примерно на **42%**.

## Гистограммы

### Разделяемая память SYS V

![Разделяемая память SYS V](results/shm_transfer.png)

### Очередь сообщений SYS V

![Очередь сообщений SYS V](results/mq_transfer.png)

### Именованный канал FIFO

![Именованный канал FIFO](results/fifo_transfer.png)
