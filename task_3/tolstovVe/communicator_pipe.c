#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "communicator.h"



int commInit(Communicator *communicator) {
	if (pipe(communicator->pipe1) != 0) {
		perror("pipe generation failed");
		return -1;
	}
	if (pipe(communicator->pipe2) != 0) {
		perror("pipe generation failed");
		return -1;
	}

	communicator->pipes[0] = &(communicator->pipe1[0]);
	communicator->pipes[1] = &(communicator->pipe2[0]);
	return 0;
}


int commDestroy(Communicator *communicator) {
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			if (close(communicator->pipes[i][j]) != 0)
				return -1;
	return 0;
}


int commWrite(Communicator *communicator, int pipeNumber, char *bits, size_t bufsize) {
	return write(communicator->pipes[pipeNumber][1], (const void *) bits, bufsize);
}


int commRead(Communicator *communicator, int pipeNumber, char *buf, size_t bufsize) {
	return read(communicator->pipes[pipeNumber][0], buf, bufsize);
}