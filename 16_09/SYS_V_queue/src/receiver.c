#include "msg_queue.h"
#include "sys_v.h"
#include "ipc_utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>

static int getMaxMessageSize(size_t *max_message_size) {
    FILE *limit_file = fopen("/proc/sys/kernel/msgmax", "r");
    if (limit_file == NULL) {
        perror("fopen msgmax");
        return -1;
    }

    if (fscanf(limit_file, "%zu", max_message_size) != 1) {
        fprintf(stderr, "Cannot read kernel.msgmax\n");
        fclose(limit_file);
        return -1;
    }

    if (fclose(limit_file) == EOF) {
        perror("fclose msgmax");
        return -1;
    }

    if (*max_message_size < QUEUE_MESSAGE_METADATA_SIZE) {
        fprintf(stderr, "Invalid maximum message size\n");
        return -1;
    }

    return 0;
}

static int receiveMessage(int msqid, struct QueueMessage *message,
                          size_t max_message_size) {
    ssize_t received_size;

    do {
        received_size = msgrcv(
            msqid,
            message,
            max_message_size,
            QUEUE_MESSAGE_TYPE,
            0
        );
    } while (received_size == -1 && errno == EINTR);

    if (received_size == -1) {
        perror("msgrcv");
        return -1;
    }

    if ((size_t)received_size < QUEUE_MESSAGE_METADATA_SIZE) {
        fprintf(stderr, "Invalid message header\n");
        return -1;
    }

    size_t payload_size =
        (size_t)received_size - QUEUE_MESSAGE_METADATA_SIZE;

    if (message->size != payload_size ||
        (!message->finished && message->size == 0)) {
        fprintf(stderr, "Invalid message size or state\n");
        return -1;
    }

    return 0;
}

static int readMessage(const struct QueueMessage *message, FILE *output) {
    if (message->size > 0 &&
        fwrite(message->data, 1, message->size, output) != message->size) {
        perror("fwrite");
        return -1;
    }

    return 0;
}

static int receiveFile(int msqid, FILE *output,
                       size_t max_message_size) {
    size_t allocation_size = sizeof(long) + max_message_size;

    struct QueueMessage *message = calloc(1, allocation_size);
    if (message == NULL) {
        perror("calloc message");
        return -1;
    }

    int result = 0;

    while (true) {
        if (receiveMessage(msqid, message, max_message_size) == -1) {
            result = -1;
            break;
        }

        if (readMessage(message, output) == -1) {
            result = -1;
            break;
        }

        if (message->finished) {
            break;
        }
    }

    free(message);
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        return 1;
    }

    size_t max_message_size;
    if (getMaxMessageSize(&max_message_size) == -1) {
        return 1;
    }

    FILE *output = fopen(argv[1], "wb");
    if (output == NULL) {
        perror("fopen output");
        return 1;
    }

    key_t queue_key = getIpcKey(IPC_KEY_FILE, QUEUE_PROJECT_ID);
    if (queue_key == (key_t)-1) {
        fclose(output);
        return 1;
    }

    int msqid = msgget(queue_key, 0);
    if (msqid == -1) {
        perror("msgget");
        fclose(output);
        return 1;
    }

    int result = receiveFile(msqid, output, max_message_size);

    if (fclose(output) == EOF) {
        perror("fclose output");
        result = -1;
    }

    if (removeMessageQueue(msqid) == -1) {
        result = -1;
    }

    return result == 0 ? 0 : 1;
}

