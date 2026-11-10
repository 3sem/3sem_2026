#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

int main(void) {
    for (;;) {
        char *p = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) return 1;
        p[0] = 1;
        munmap(p, 4096);

        printf("%d\n", getpid());
        fflush(stdout);
        sleep(1);
    }
}
