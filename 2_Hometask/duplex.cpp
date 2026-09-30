#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "duplex.h"
#include "process.h"
#include "fileFunctions.h"

duplex_t* duplexCtor () {
    duplex_t* duplex = (duplex_t*)calloc(1, sizeof(duplex_t));
    if (!duplex) {
        perror("Failed duplex calloc.");
        return NULL;
    }

    duplex->pid = -1;

    duplex->parent = processCtor();
    if (!duplex->parent) {
        duplexDtor(duplex);
        return NULL;
    }

    duplex->child = processCtor();
    if (!duplex->child) {
        duplexDtor(duplex);
        return NULL;
    }

    if (connectProcesses(duplex->parent, duplex->child) == -1) {
        duplexDtor(duplex);
        return NULL;
    }

    if ((duplex->pid = fork()) == -1) {
        perror("Failed fork.");
        duplexDtor(duplex);
        return NULL;
    }

    if (duplex->pid == 0) {
        processDtor(duplex->parent);
        duplex->parent = NULL;
    } else {
        processDtor(duplex->child);
        duplex->child = NULL;
    }

    return duplex;
}

void duplexDtor (duplex_t* duplex) {
    if (!duplex) {
        return;
    }

    processDtor(duplex->parent);
    processDtor(duplex->child);

    if (duplex->pid > 0) {
        pid_t result;

        do {
            result = wait(NULL);
        } while (result == -1 && errno == EINTR);

        if (result == -1) {
            perror("wait");
        }
    }

    free(duplex);
}

int childEcho(process_t* child) {
    assert(child);

    char buf[BUF_SIZE] = {};
    ssize_t size = 0;

    while ((size = processRead(child, buf, sizeof(buf))) > 0) {
        if (processWrite(child, buf, (size_t)size) == -1) {
            return -1;
        }
    }

    return size == -1 ? -1 : 0;
}

int parentEcho(process_t* parent) {
    assert(parent);

    char buf[BUF_SIZE] = {};
    ssize_t size = 0;

    while ((size = readFull(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        int lastBlock = (size_t)size < sizeof(buf);

        if (processExchange(parent, buf, (size_t)size, lastBlock) == -1)
            return -1;

        if (writeFull(STDOUT_FILENO, buf, (size_t)size) == -1)
            return -1;

        if (lastBlock)
            return 0;
    }

    processFinishWrite(parent);
    return size == -1 ? -1 : 0;
}
