Толстов Виктор Б01-506
Инструкции по компиляции лежат внутри task3.c, вот копирую сюда тоже

Я вам показывал это задание с разными размерами буффера, запущенное на компе Глеба Овсиенко, но там были странные исключительные случаи с расхождением по времени в несколько раз, поэтому вы попросили прислать исходники. Вот они.

/*
	- - - INSTRUCTIONS - - -

To compile, use either:

gcc task3.c communicator_pipe.c // will compile with pipe as the method of data transfer
gcc task3.c communicator_sysv_shared.c // with shared memory
gcc task3.c communicator_sysv_queue.c // with message queue

Working files:
	task3.c
	communicator.h
+any one:
	1. communicator_pipe.c
	2. communicator_sysv_shared.c
	3. communicator_sysv_queue.c

To change buffer size, redefine:

	BUFSIZE in task3.c (in this file)
	MSGSIZE in communicator_sysv_queue.c
	SHM_SIZE in communicator_sysv_shared.c

	communicator_pipe.c does not rely on any buffers, so no need to change anything

Input file: data

Output file: datafinal

NOTE: known bug (harmful) - if "datafinal" doesn't exist in the folder,
dup2() will fail silently and the text will be dumped into your terminal instead.
Considering that file size is usually 4GB, your terminal may lag the system.
WSL froze entirely this way and I had to restart the PC. So create it beforehand.

NOTE2: process sleeps for 1 additional second in the end before destroying the communicator
*/
