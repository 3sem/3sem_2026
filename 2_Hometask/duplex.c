#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "duplex.h"
#include "fileFunctions.h"

duplex_t* duplexCtor () {
    duplex_t* duplex = (duplex_t*)calloc(1, sizeof(duplex_t));
    if (!duplex) {
        perror("Failed duplex calloc.");
        return NULL;
    }

    duplex->pid = -1;
    duplex->toChild[0] = duplex->toChild[1] = INIT_FD;
    duplex->toParent[0] = duplex->toParent[1] = INIT_FD;

    duplex->ops.read = duplexRead;
    duplex->ops.write = duplexWrite;
    duplex->ops.finishWrite = duplexFinishWrite;

    if (pipe(duplex->toChild) == -1) {
        duplexDtor(duplex);
        return NULL;
    }

    if (pipe(duplex->toParent) == -1) {
        perror("pipe to parent");
        duplexDtor(duplex);
        return NULL;
    }

    if ((duplex->pid = fork()) == -1) {
        perror("Failed fork.");
        duplexDtor(duplex);
        return NULL;
    }

    if (duplex->pid == 0) {
    closeFd(&duplex->toChild[1]);
    closeFd(&duplex->toParent[0]);
    } else {
        closeFd(&duplex->toChild[0]);
        closeFd(&duplex->toParent[1]);
    }

    return duplex;
}

void duplexDtor (duplex_t* duplex) {
    if (!duplex) {
        return;
    }

    closeFd(&duplex->toChild[0]);
    closeFd(&duplex->toChild[1]);
    closeFd(&duplex->toParent[0]);
    closeFd(&duplex->toParent[1]);

    if (duplex->pid > 0) {
        pid_t result;

        do {
            result = waitpid(duplex->pid, NULL, 0);
        } while (result == -1 && errno == EINTR);

        if (result == -1) {
            perror("wait");
        }
    }

    free(duplex);
}

int childEcho(duplex_t* duplex) {
    assert(duplex);

    char buf[BUF_SIZE] = {};
    ssize_t size = 0;

    while ((size = duplex->ops.read(duplex, buf, sizeof(buf))) > 0) {
        if (duplex->ops.write(duplex, buf, (size_t)size) == -1) {
            return -1;
        }
    }

    return size == -1 ? -1 : 0;
}

int parentEcho(duplex_t* duplex) {
    assert(duplex);

    char buf[BUF_SIZE] = {};
    ssize_t size = 0;

    while ((size = readFull(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        int lastBlock = (size_t)size < sizeof(buf);

        if (duplexExchange(duplex, buf, (size_t)size, lastBlock) == -1)
            return -1;

        if (writeFull(STDOUT_FILENO, buf, (size_t)size) == -1)
            return -1;

        if (lastBlock)
            return 0;
    }

    duplex->ops.finishWrite(duplex);
    return size == -1 ? -1 : 0;
}

ssize_t duplexRead(duplex_t* duplex, char* buf, size_t size) {
    assert(duplex);

    int fd = duplex->pid == 0
        ? duplex->toChild[0]
        : duplex->toParent[0];

    return readFull(fd, buf, size);
}

int duplexWrite(duplex_t* duplex, const char* buf, size_t size) {
    assert(duplex);

    int fd = duplex->pid == 0
        ? duplex->toParent[1]
        : duplex->toChild[1];

    return writeFull(fd, buf, size);
}

void duplexFinishWrite(duplex_t* duplex) {
    assert(duplex);

    int* fd = duplex->pid == 0
        ? &duplex->toParent[1]
        : &duplex->toChild[1];

    closeFd(fd);
}

int duplexExchange(duplex_t* duplex, char* buf, size_t size, int lastBlock) {
    assert(duplex);
    assert(buf);

    if (duplex->ops.write(duplex, buf, size) == -1) {
        return -1;
    }

    if (lastBlock) {
        duplex->ops.finishWrite(duplex);
    }

    ssize_t received = duplex->ops.read(duplex, buf, size);

    if (received == -1)
        return -1;

    if ((size_t)received != size) {
        fprintf(stderr, "Error: incomplete echo.\n");
        return -1;
    }

    return 0;
}
