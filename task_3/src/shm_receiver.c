#include "common.h"

#include <sys/shm.h>
#include <sys/sem.h>

#define SEM_EMPTY 0
#define SEM_FULL 1

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static void usage(void)
{
    fprintf(stderr, "использование: shm_receiver <выходной_файл> <размер_буфера>\n");
    exit(2);
}

static int create_shm(key_t key, size_t size)
{
    int id = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666);
    if (id >= 0)
        return id;
    if (errno != EEXIST)
        die("shmget");
    id = shmget(key, 0, 0666);
    if (id >= 0 && shmctl(id, IPC_RMID, NULL) < 0)
        die("shmctl IPC_RMID");
    id = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666);
    if (id < 0)
        die("shmget");
    return id;
}

static int create_sem(key_t key)
{
    int id = semget(key, 2, IPC_CREAT | IPC_EXCL | 0666);
    if (id < 0 && errno == EEXIST) {
        id = semget(key, 0, 0666);
        if (id >= 0 && semctl(id, 0, IPC_RMID) < 0)
            die("semctl IPC_RMID");
        id = semget(key, 2, IPC_CREAT | IPC_EXCL | 0666);
    }
    if (id < 0)
        die("semget");
    return id;
}

static void sem_set(int id, int num, int val)
{
    union semun arg;
    arg.val = val;
    if (semctl(id, num, SETVAL, arg) < 0)
        die("semctl SETVAL");
}

static void sem_op(int id, unsigned short num, short op)
{
    struct sembuf sb;

    sb.sem_num = num;
    sb.sem_op = op;
    sb.sem_flg = 0;
    while (semop(id, &sb, 1) < 0) {
        if (errno == EINTR)
            continue;
        die("semop");
    }
}

int main(int argc, char **argv)
{
    size_t bufsize;
    size_t region;
    int out_fd;
    int shmid;
    int semid;
    int seen;
    uint64_t filesize;
    uint64_t got;
    uint64_t t0;
    uint64_t t1;
    struct shm_slot *slot;

    if (argc != 3)
        usage();

    bufsize = parse_bufsize(argv[2]);
    out_fd = open_output(argv[1]);
    region = sizeof(struct shm_slot) + bufsize;

    shmid = create_shm(make_key(PROJ_SHM), region);
    slot = shmat(shmid, NULL, SHM_RDONLY);
    if (slot == (void *)-1)
        die("shmat");

    semid = create_sem(make_key(PROJ_SEM));
    /* Писатель может занять слот сразу, читатель ждёт данные. */
    sem_set(semid, SEM_EMPTY, 1);
    sem_set(semid, SEM_FULL, 0);

    fprintf(stderr, "READY\n");
    fflush(stderr);

    seen = 0;
    filesize = 0;
    got = 0;
    t0 = 0;
    t1 = 0;

    for (;;) {
        sem_op(semid, SEM_FULL, -1);
        if (!seen) {
            seen = 1;
            filesize = slot->filesize;
            t0 = slot->t0_ns;
            if (filesize == 0) {
                t1 = monotonic_ns();
                sem_op(semid, SEM_EMPTY, 1);
                break;
            }
        }

        write_full(out_fd, slot->data, slot->len);
        got += slot->len;
        if (got >= filesize) {
            t1 = monotonic_ns();
            sem_op(semid, SEM_EMPTY, 1);
            break;
        }
        sem_op(semid, SEM_EMPTY, 1);
    }

    if (got != filesize) {
        fprintf(stderr, "принято %" PRIu64 " байт, ожидалось %" PRIu64 "\n",
                got, filesize);
        exit(1);
    }

    print_report("shm", bufsize, got, t0, t1);

    close(out_fd);
    shmdt(slot);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    return 0;
}
