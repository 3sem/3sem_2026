#include "common.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_PATH_SIZE 64

typedef struct fifo_t {
    int readFd;
    int writeFd;
    char path[FIFO_PATH_SIZE];
} fifo_t;

void removeFifo(fifo_t* fifo) {
    closeFd(&fifo->readFd);
    closeFd(&fifo->writeFd);
    unlink(fifo->path);
}

int createFifo(fifo_t* fifo) {
    snprintf(fifo->path, sizeof(fifo->path), "./pid_%ld.fifo", (long)getpid());
    if (mkfifo(fifo->path, 0600) == -1) {
        perror("mkfifo");
        return -1;
    }

    fifo->readFd = open(fifo->path, O_RDONLY | O_NONBLOCK);
    if (fifo->readFd >= 0)
        fifo->writeFd = open(fifo->path, O_WRONLY);

    int flags = fifo->readFd < 0 ? -1 : fcntl(fifo->readFd, F_GETFL);
    if (fifo->writeFd < 0 || flags < 0 ||
        fcntl(fifo->readFd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        perror("open FIFO");
        removeFifo(fifo);
        return -1;
    }

    return 0;
}

int transferFile(size_t bufferSize) {
    fifo_t fifo = { .readFd = -1, .writeFd = -1 };
    if (createFifo(&fifo) == -1)
        return -1;

    pid_t childPid = fork();
    if (childPid == -1) {
        perror("fork");
        removeFifo(&fifo);
        return -1;
    }

    if (childPid == 0) {
        closeFd(&fifo.writeFd);

        int result = copyFile(fifo.readFd, STDOUT_FILENO, bufferSize);

        closeFd(&fifo.readFd);
        _exit(result == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }


    closeFd(&fifo.readFd);

    int sendResult = copyFile(STDIN_FILENO, fifo.writeFd, bufferSize);

    closeFd(&fifo.writeFd);

    int receiveResult = waitChild(childPid);
    removeFifo(&fifo);
    return sendResult == -1 || receiveResult == -1 ? -1 : 0;
}
