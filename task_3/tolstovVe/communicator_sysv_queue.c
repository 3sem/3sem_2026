#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "communicator.h"

#define MSGSIZE 4096

typedef struct {
	long mtype;
	char mtext[MSGSIZE];
} Msgbuf;


int commInit(Communicator *communicator) {
	int key = IPC_PRIVATE;
	if ((communicator->shmid = msgget(key, IPC_CREAT | 0666)) < 0) {
		perror("msgget\n");
		return 1;
	}
	
	printf("msgget success\n");
	return 0;
}


int commDestroy(Communicator *communicator) {
	communicator->shmid = 0;
	// TODO - make destructor
	return 0;
}
int commWrite(Communicator *communicator, int pipeNumber, char *bits, size_t bufsize) {
	//sleep(1);
	//printf("sending...\n");
	Msgbuf sbuf = {};
	sbuf.mtype = 1;
	int size = bufsize;
	int i = 0;
	for (; i < size && i < MSGSIZE - 1; i++) {
		sbuf.mtext[i] = bits[i];
	}
	for (int j = i+1; j < MSGSIZE; j++) {
		sbuf.mtext[j] = 'x';
	}
	sbuf.mtext[size] = '\0';

	//printf("%s\n", sbuf.mtext);

	if (msgsnd(communicator->shmid, &sbuf, MSGSIZE, 0) < 0) {
		//printf("%d, %ld, %s, %d\n", communicator->shmid, sbuf.mtype, sbuf.mtext, size);
		perror("msgsnd");
		exit(1);
	}
	return size;
}

int commRead(Communicator *communicator, int pipeNumber, char *buf, size_t bufsize) {
	Msgbuf rbuf = {};
	rbuf.mtype = 1;
	int i = 0;
	int j = MSGSIZE;
	

	if (msgrcv(communicator->shmid, &rbuf, MSGSIZE, 0, 0) < 0) {
		return 0;
	}
	//fprintf(stderr, "receiving...\n");
	
	/*for (int k = 0; k < MSGSIZE; k++) {
		fprintf(perror, "%c|", rbuf.mtext[k]);
	}*/
	while (--j > 0 && rbuf.mtext[j] != '\0');
	for (i = 0; i < j; i++) {
		buf[i] = rbuf.mtext[i];
	}
	
	//fprintf(stderr, "|%s| %d\n", rbuf.mtext, i);

	return j;
}