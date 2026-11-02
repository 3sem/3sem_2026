#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "communicator.h"

#define BUFSIZE 4096
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

void send(const char *FILENAME_IN, const char *FILENAME_OUT);//


int main() {
	const char FILENAME_IN[] = "data";
	const char FILENAME_OUT[] = "datafinal";
	
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	send(FILENAME_IN, FILENAME_OUT);//send FILENAME_IN to FILENAME_OUT
	clock_gettime(CLOCK_MONOTONIC, &end);
	printf("%lf\n", (double) end.tv_sec - start.tv_sec + (double) (end.tv_nsec - start.tv_nsec) / 1000000000);

	return 0;
}


void send(const char *FILENAME_IN, const char *FILENAME_OUT) {
	int size = 0;
	char buf[/*10*/BUFSIZE];

	//int fd2 = open("datafinaldouble", O_WRONLY);
	
	Communicator communicator = {};
	
	if (commInit(&communicator)) {
		perror("Communicator init failed\n");
		return;
	}

	const pid_t pid = fork();
	
	if (pid < 0) {
		perror("fork() error\n");
		return;
	}

	
	if (pid) {//parent

		int fd = open(FILENAME_IN, O_RDONLY);

		int endflag = 0;

		while (1) {
			//perror("Parent is here\n");
			size = read(fd, buf, sizeof(buf)-1);
			if (size == -1) {
				perror("error while reading file");
				break;
			}
			buf[size] = '\0';

			if (size <= 0) {
				fprintf(stderr, "File empty %d\n", size);
				break;
			}

			//write(fd2, buf, size);
			//fprintf(stderr, "Trying to send %d |%s|\n", size, buf);
			if ((size = commWrite(&communicator, 0, buf, size)) <= 0) {
				fprintf(stderr, "Intended parental death %d |%s|\n", size, buf);
				read(fd, buf, 1);
				fprintf(stderr, "|%d|\n", buf[0]);
				break;
			}
		}
		
		sleep(1);
		commDestroy(&communicator);
		close(fd);
		return;

	} else {//child
		int fd = open(FILENAME_OUT, O_WRONLY);
		dup2(fd, 1);//replace stdout with FILENAME_OUT file

		while (1) {
			if ((size = commRead(&communicator, 0, buf, sizeof(buf)-1)) <= 0)
				exit(0);
			buf[size] = '\0';
			//fprintf(stderr, "Received by child: %s |%d|\n", buf, size);
			
			write(1, buf, size);
		}
		
		close(fd);

		exit(0);
	}
}
