#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        return 1;
    }

    char *end = NULL;
    errno = 0;
    long pid_l = strtol(argv[1], &end, 10);
    if (errno != 0 || *end != '\0' || pid_l <= 0 || pid_l > 0x7fffffff) {
        fprintf(stderr, "Invalid PID: %s\n", argv[1]);
        return 1;
    }
    int pid = (int)pid_l;

    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    while (kill(pid, 0) == 0 || errno == EPERM) {
        FILE *f = fopen(path, "r");
        if (!f) break;

        char comm[256];
        char state;
        unsigned long minflt, majflt;

        int n = fscanf(f, "%*d (%255[^)]) %c %*d %*d %*d %*d %*d %*u %lu %*u %lu",
                       comm, &state, &minflt, &majflt);
        fclose(f);

        
        printf("state=%c minflt=%lu majflt=%lu\n", state, minflt, majflt);
       
	    
        

        usleep(500000); /* 0.5 s */
    }

    return 0;
}
