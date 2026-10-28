#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

enum { BUFFER_FREE, BUFFER_READY };

typedef struct shared_buffer_t {
    size_t dataSize;
    unsigned char data[];
} shared_buffer_t;

typedef struct shared_memory_t {
    int memoryId;
    int semaphoreId;
    shared_buffer_t* buffer;
} shared_memory_t;

union semun { unsigned short* array; };

void removeSharedMemory(shared_memory_t* memory) {
    if (memory->buffer)
        shmdt(memory->buffer);
    if (memory->semaphoreId >= 0)
        semctl(memory->semaphoreId, 0, IPC_RMID);
    if (memory->memoryId >= 0)
        shmctl(memory->memoryId, IPC_RMID, NULL);
}

int createSharedMemory(shared_memory_t* memory, size_t bufferSize) {
    memory->memoryId = shmget(IPC_PRIVATE, sizeof(*memory->buffer) + bufferSize,
                              IPC_CREAT | 0600);
    if (memory->memoryId >= 0) {
        memory->buffer = shmat(memory->memoryId, NULL, 0);
        if (memory->buffer == (void*)-1)
            memory->buffer = NULL;
    }

    if (!memory->buffer) {
        perror("shared memory");
        removeSharedMemory(memory);
        return -1;
    }

    memory->semaphoreId = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    unsigned short values[] = { 1, 0 };
    union semun argument = { .array = values };
    if (memory->semaphoreId < 0 || semctl(memory->semaphoreId, 0, SETALL, argument) == -1) {
        perror("semaphores");
        removeSharedMemory(memory);
        return -1;
    }

    return 0;
}

int changeSemaphore(int semaphoreId, unsigned short semNum, short change) {
    struct sembuf operation = { semNum, change, 0 };
    int result = 0;

    do {
        result = semop(semaphoreId, &operation, 1);
    } while (result == -1 && errno == EINTR);

    if (result == -1)
        perror("semop");

    return result;
}

int sendFile(shared_memory_t* memory, size_t bufferSize) {
    while (1) {
        if (changeSemaphore(memory->semaphoreId, BUFFER_FREE, -1) == -1)
            return -1;

        ssize_t dataSize = readChunk(STDIN_FILENO, memory->buffer->data, bufferSize);
        if (dataSize == -1)
            return -1;

        memory->buffer->dataSize = (size_t)dataSize;

        if (changeSemaphore(memory->semaphoreId, BUFFER_READY, 1) == -1)
            return -1;

        if (dataSize == 0)
            return 0;
    }
}

int receiveFile(shared_memory_t* memory) {
    while (1) {
        if (changeSemaphore(memory->semaphoreId, BUFFER_READY, -1) == -1)
            return -1;

        if (memory->buffer->dataSize == 0)
            return 0;

        if (writeFull(STDOUT_FILENO, memory->buffer->data, memory->buffer->dataSize) == -1)
            return -1;

        if (changeSemaphore(memory->semaphoreId, BUFFER_FREE, 1) == -1)
            return -1;
    }
}

int transferFile(size_t bufferSize) {
    shared_memory_t memory = { .memoryId = -1, .semaphoreId = -1 };
    if (createSharedMemory(&memory, bufferSize) == -1)
        return -1;

    pid_t childPid = fork();
    if (childPid == -1) {
        perror("fork");
        removeSharedMemory(&memory);
        return -1;
    }

    if (childPid == 0) {
        int result = receiveFile(&memory);
        if (result == -1)
            semctl(memory.semaphoreId, 0, IPC_RMID);
        shmdt(memory.buffer);
        _exit(result == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    int sendResult = sendFile(&memory, bufferSize);
    if (sendResult == -1)
        semctl(memory.semaphoreId, 0, IPC_RMID);

    int receiveResult = waitChild(childPid);

    removeSharedMemory(&memory);
    return (sendResult == -1 || receiveResult == -1) ? -1 : 0;
}
