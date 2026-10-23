#include "common.h"
#include <math.h>
#include <sys/wait.h>
#include <unistd.h>

static int shmid = -1;
static Shared *shared = (void *)-1;

static void cleanup(void)
{
    if (shared != (void *)-1)
        shmdt(shared);
    if (shmid != -1)
        shmctl(shmid, IPC_RMID, NULL);
}

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s points threads [seed]\n", argv[0]);
        return 1;
    }
    uint64_t points = number(argv[1]), threads = number(argv[2]);
    uint64_t seed = argc == 4 ? number(argv[3]) : 42;
    if (!points || points > UINT64_MAX / 2 || !threads || threads > points ||
        threads > 65536 || threads > (SIZE_MAX - sizeof(Shared)) / sizeof(uint64_t)) {
        fprintf(stderr, "Need 1 <= threads <= min(points, 65536), 1 <= points <= 2^63-1\n");
        return 1;
    }
    if (atexit(cleanup) != 0)
        return 1;
    size_t size = sizeof(Shared) + (size_t)threads * sizeof(uint64_t);
    shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
    if (shmid == -1)
        fail("shmget");
    shared = shmat(shmid, NULL, 0);
    if (shared == (void *)-1)
        fail("shmat");
    shared->points = points;
    shared->threads = (size_t)threads;
    shared->seed = seed;

    char id[32];
    snprintf(id, sizeof(id), "%d", shmid);
    pid_t child = fork();
    if (child == -1)
        fail("fork");
    if (child == 0) {
        execl("./a", "a", id, (char *)NULL);
        perror("exec ./a (run from the task directory)");
        _exit(1);
    }
    int status;
    pid_t result;
    do {
        result = waitpid(child, &status, 0);
    } while (result == -1 && errno == EINTR);
    if (result == -1)
        fail("waitpid");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Program a failed\n");
        return 1;
    }

    uint64_t hits = 0;
    for (size_t i = 0; i < shared->threads; ++i)
        hits += shared->hits[i];
    double integral = (double)hits / points;
    printf("points,threads,seed,hits,integral,error,seconds\n");
    printf("%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.10f,%.10f,%.9f\n",
           points, threads, seed, hits, integral, fabs(integral - 1.0 / 3.0), shared->seconds);
    return 0;
}
