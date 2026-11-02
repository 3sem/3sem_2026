typedef struct
{
	int *pipes[2];
	int pipe1[2];
	int pipe2[2];

	char *shm;
	int shmid;
	int sem;

	/*int (*commInit)(Communicator *communicator);
	int (*commDestroy)(Communicator *communicator);
	int (*commWrite)(Communicator *communicator, int pipeNumber, char *bits, size_t bufsize);
	int (*commRead)(Communicator *communicator, int pipeNumber, char *buf, size_t bufsize);*/
} Communicator;

int commInit(Communicator *communicator);
int commDestroy(Communicator *communicator);
int commWrite(Communicator *communicator, int pipeNumber, char *bits, size_t bufsize);
int commRead(Communicator *communicator, int pipeNumber, char *buf, size_t bufsize);