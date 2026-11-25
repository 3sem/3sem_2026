#include "monte_carlo_common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

int parseUint64(const char *text, uint64_t *value) {
    if (text == NULL || *text == '\0' || value == NULL) {
        return -1;
    }
    for (const char *digit = text; *digit != '\0'; ++digit) {
        if (*digit < '0' || *digit > '9') {
            return -1;
        }
    }

    errno = 0;
    char *end = NULL;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' || parsed == 0) {
        return -1;
    }
    *value = (uint64_t)parsed;
    return 0;
}

int parseMode(const char *text, mc_mode_t *mode) {
    if (text == NULL || mode == NULL) {
        return -1;
    }
    if (strcmp(text, "per_thread") == 0) {
        *mode = MC_MODE_PER_THREAD;
        return 0;
    }
    if (strcmp(text, "shared_mutex") == 0) {
        *mode = MC_MODE_SHARED_MUTEX;
        return 0;
    }
    return -1;
}

const char *modeName(mc_mode_t mode) {
    switch (mode) {
        case MC_MODE_PER_THREAD: return "per_thread";
        case MC_MODE_SHARED_MUTEX: return "shared_mutex";
        default: return "unknown";
    }
}

key_t getIpcKey(void) {
    int fd = open(MC_KEY_FILE, O_CREAT | O_RDWR, 0600);
    if (fd == -1) {
        perror("open IPC key file");
        return (key_t)-1;
    }
    if (close(fd) == -1) {
        perror("close IPC key file");
        return (key_t)-1;
    }
    key_t key = ftok(MC_KEY_FILE, MC_PROJECT_ID);
    if (key == (key_t)-1) {
        perror("ftok");
    }
    return key;
}

int removeSharedMemoryByKey(key_t key) {
    int shmid = shmget(key, 1, 0600);
    if (shmid == -1) {
        if (errno == ENOENT) {
            return 0;
        }
        perror("shmget for cleanup");
        return -1;
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
        return -1;
    }
    return 0;
}
