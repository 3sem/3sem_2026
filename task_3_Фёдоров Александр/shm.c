#include "common.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

static int memory_id = -1, semaphore_id = -1;
static struct Shared {
    size_t size;
    unsigned char data[];
} *shared = (void *)-1;

static void cleanup(void)
{
    if (shared != (void *)-1) {
        shmdt(shared);
        shared = (void *)-1;
    }
    if (memory_id != -1) {
        shmctl(memory_id, IPC_RMID, NULL);
        memory_id = -1;
    }
    if (semaphore_id != -1) {
        semctl(semaphore_id, 0, IPC_RMID);
        semaphore_id = -1;
    }
}

static void change(unsigned short number, short value)
{
    struct sembuf operation = {number, value, 0};
    int result;
    do {
        result = semop(semaphore_id, &operation, 1);
    } while (result == -1 && errno == EINTR);
    if (result == -1)
        fail("semop");
}

int main(int argc, char **argv)
{
    prepare(argc, argv);
    memory_id = shmget(IPC_PRIVATE, sizeof(*shared) + block_size, IPC_CREAT | 0600);
    if (memory_id == -1)
        fail("shmget");
    shared = shmat(memory_id, NULL, 0);
    if (shared == (void *)-1)
        fail("shmat");
    semaphore_id = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    if (semaphore_id == -1)
        fail("semget");
    unsigned short values[2] = {1, 0}; /* 0 — свободный буфер, 1 — готовые данные. */
    union { int val; struct semid_ds *buf; unsigned short *array; } arg;
    arg.array = values;
    if (semctl(semaphore_id, 0, SETALL, arg) == -1)
        fail("semctl");

    double start = now();
    child = fork();
    if (child == -1)
        fail("fork");
    if (child == 0) {
        close(input);
        for (;;) {
            change(1, -1);
            if (shared->size == 0)
                break;
            write_all(output, shared->data, shared->size);
            change(0, 1);
        }
        shmdt(shared);
        if (close(output) == -1)
            fail("close");
        _exit(0);
    }
    close(output);
    for (;;) {
        change(0, -1);
        shared->size = (size_t)read_block(input, shared->data, block_size);
        change(1, 1);
        if (shared->size == 0)
            break;
    }
    close(input);
    finish(start);
    return 0;
}
