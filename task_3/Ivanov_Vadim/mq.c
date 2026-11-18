#define _POSIX_C_SOURCE 200809L
#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>

struct message {
    long type;
    unsigned char data[];
};

static int send_message(int queue, struct message *message, size_t length)
{
    int result;
    do {
        result = msgsnd(queue, message, length, 0);
    } while (result == -1 && errno == EINTR);
    return result;
}

static int receive_file(int queue, struct message *message, size_t message_size,
                        unsigned char *buffer, const struct options *options)
{
    size_t used = 0;
    for (;;) {
        ssize_t count;
        do {
            count = msgrcv(queue, message, message_size, 1, 0);
        } while (count == -1 && errno == EINTR);
        if (count == -1) {
            perror("msgrcv");
            return 1;
        }
        if (count == 0)
            break;
        size_t offset = 0;
        while (offset < (size_t)count) {
            size_t portion = (size_t)count - offset;
            if (portion > options->buffer_size - used)
                portion = options->buffer_size - used;
            memcpy(buffer + used, message->data + offset, portion);
            used += portion;
            offset += portion;
            if (used == options->buffer_size) {
                if (write_all(options->output, buffer, used) == -1)
                    goto write_error;
                used = 0;
            }
        }
    }
    if (write_all(options->output, buffer, used) == -1)
        goto write_error;
    return 0;

write_error:
    perror("write destination");
    msgctl(queue, IPC_RMID, NULL);
    return 1;
}

int main(int argc, char **argv)
{
    struct options options;
    if (open_files(argc, argv, &options) == -1)
        return 1;
    int queue = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (queue == -1) {
        perror("msgget");
        return 1;
    }
    size_t message_size = 8192;
    FILE *limit = fopen("/proc/sys/kernel/msgmax", "r");
    if (limit != NULL) {
        if (fscanf(limit, "%zu", &message_size) != 1)
            message_size = 8192;
        fclose(limit);
    }
    struct msqid_ds information;
    if (msgctl(queue, IPC_STAT, &information) == -1) {
        perror("msgctl");
        msgctl(queue, IPC_RMID, NULL);
        return 1;
    }
    if (message_size > information.msg_qbytes)
        message_size = information.msg_qbytes;
    if (message_size > options.buffer_size)
        message_size = options.buffer_size;
    if (message_size == 0) {
        fprintf(stderr, "Message queue capacity is zero\n");
        msgctl(queue, IPC_RMID, NULL);
        return 1;
    }
    struct message *message = malloc(sizeof(*message) + message_size);
    unsigned char *buffer = malloc(options.buffer_size);
    if (message == NULL || buffer == NULL) {
        perror("malloc");
        free(message);
        free(buffer);
        msgctl(queue, IPC_RMID, NULL);
        return 1;
    }
    message->type = 1;
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    pid_t child = fork();
    int result = 0;
    if (child == -1) {
        perror("fork");
        result = 1;
    } else if (child == 0) {
        close(options.input);
        result = receive_file(queue, message, message_size, buffer, &options);
        if (close(options.output) == -1)
            result = 1;
        free(message);
        free(buffer);
        _exit(result);
    } else {
        close(options.output);
        ssize_t count;
        while ((count = read_retry(options.input, buffer, options.buffer_size)) > 0) {
            size_t offset = 0;
            while (offset < (size_t)count) {
                size_t portion = (size_t)count - offset;
                if (portion > message_size)
                    portion = message_size;
                memcpy(message->data, buffer + offset, portion);
                if (send_message(queue, message, portion) == -1) {
                    perror("msgsnd");
                    result = 1;
                    break;
                }
                offset += portion;
            }
            if (result != 0)
                break;
        }
        if (count == -1) {
            perror("read source");
            result = 1;
        }
        if (result == 0 && send_message(queue, message, 0) == -1) {
            perror("msgsnd");
            result = 1;
        }
        if (result != 0)
            msgctl(queue, IPC_RMID, NULL);
        close(options.input);
        if (wait_child(child) == -1)
            result = 1;
    }
    msgctl(queue, IPC_RMID, NULL);
    free(message);
    free(buffer);
    if (result == 0)
        print_time(&start);
    return result;
}
