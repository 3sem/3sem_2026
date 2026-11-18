#include "transfer.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <unistd.h>

size_t probeMessageLimit(int queue_id) {
    struct msqid_ds queue_info;
    if (msgctl(queue_id, IPC_STAT, &queue_info) < 0) {
        return 0;
    }
    size_t upper = (size_t) queue_info.msg_qbytes;
    if (upper > MAX_BUFFER_SIZE) {
        upper = MAX_BUFFER_SIZE;
    }
    struct ProbeMessage {
        long type;
        unsigned char data[];
    };
    struct ProbeMessage *message = calloc(1, sizeof(*message) + upper);
    if (message == NULL) {
        return 0;
    }
    message->type = 1;
    size_t lower = 0;
    /* The empty queue is private. Never modify system-wide IPC limits. */
    while (lower < upper) {
        size_t trial = lower + (upper - lower + 1) / 2;
        if (msgsnd(queue_id, message, trial, IPC_NOWAIT) == 0) {
            if (msgrcv(queue_id, message, trial, 1, IPC_NOWAIT) != (ssize_t) trial) {
                lower = 0;
                break;
            }
            lower = trial;
        } else if (errno == EINVAL) {
            upper = trial - 1;
        } else {
            lower = 0;
            break;
        }
    }
    int saved_errno = errno;
    free(message);
    errno = saved_errno;
    return lower;
}

int openFifo(int fifo_fds[2]) {
    char directory[] = "/tmp/task3_fifo_XXXXXX";
    char path[sizeof(directory) + 8];
    fifo_fds[0] = -1;
    fifo_fds[1] = -1;
    if (mkdtemp(directory) == NULL) {
        return -1;
    }
    snprintf(path, sizeof(path), "%s/data", directory);
    if (mkfifo(path, 0600) < 0) {
        int saved_errno = errno;
        rmdir(directory);
        errno = saved_errno;
        return -1;
    }
    fifo_fds[0] = open(path, O_RDONLY | O_NONBLOCK);
    if (fifo_fds[0] >= 0) {
        fifo_fds[1] = open(path, O_WRONLY | O_NONBLOCK);
    }
    int saved_errno = errno;
    int unlink_result = unlink(path);
    int rmdir_result = rmdir(directory);
    if (fifo_fds[0] < 0 || fifo_fds[1] < 0 || unlink_result < 0 || rmdir_result < 0) {
        if (fifo_fds[0] >= 0) {
            close(fifo_fds[0]);
        }
        if (fifo_fds[1] >= 0) {
            close(fifo_fds[1]);
        }
        errno = saved_errno;
        return -1;
    }
    /* Open descriptors retain this named FIFO after unlink, including at fork. */
    return 0;
}

long probeFifoCapacity(void) {
    int fifo_fds[2];
    if (openFifo(fifo_fds) < 0) {
        return -1;
    }
    long capacity = 0;
#ifdef F_GETPIPE_SZ
    capacity = fcntl(fifo_fds[0], F_GETPIPE_SZ);
#else
    unsigned char buffer[4096] = {0};
    for (;;) {
        ssize_t count = write(fifo_fds[1], buffer, sizeof(buffer));
        if (count > 0) {
            capacity += (long) count;
        } else if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            capacity = -1;
            break;
        }
    }
#endif
    int saved_errno = errno;
    close(fifo_fds[0]);
    close(fifo_fds[1]);
    errno = saved_errno;
    return capacity;
}
