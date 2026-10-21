#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        return 1;
    }
    int pid = atoi(argv[1]);
    char path[4096];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    while (kill(pid, 0) == 0) { // проверка, жив ли процесс
        FILE *f = fopen(path, "r");
        if (!f) break;
        // формат записи в "/proc/%d/stat" специфический -- применим для его чтения соответствующую форматную строку
        // Поле 2 (comm) может содержать пробелы и скобки,
        // поэтому читаем его отдельно, а дальше — по счётчикам.
        char comm[256];
        char state;
        unsigned long minflt, majflt;

        // %*d — pid, %s — (comm) -- имя процесса в скобках ,
        // %c — state, затем пропускаем ppid, pgrp, session, tty_nr, tpgid,
        // flags, minflt(10), cminflt(11), majflt(12)
        if (fscanf(f, "%*d (%255[^)]) %c %*d %*d %*d %*d %*d %*u %lu %*u %lu",
                   comm, &state, &minflt, &majflt) == 4) {
            printf("%lu %lu\n", minflt, majflt);
        }
        fclose(f);
        usleep(500000); // 0.5 s
    }
    return 0;
}
