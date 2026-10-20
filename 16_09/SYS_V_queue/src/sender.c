#include "msg_queue.h"
#include "sys_v.h"
#include "ipc_utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>

static int getMaximumPayloadSize(int msqid, size_t *max_payload_size) {
    FILE *limit_file = fopen("/proc/sys/kernel/msgmax", "r");
    if (limit_file == NULL) {
        perror("fopen msgmax");
        return -1;
    }

    size_t kernel_limit;
    if (fscanf(limit_file, "%zu", &kernel_limit) != 1) {
        fprintf(stderr, "Failed to read kernel.msgmax\n");
        fclose(limit_file);
        return -1;
    }

    if (fclose(limit_file) == EOF) {
        perror("fclose msgmax");
        return -1;
    }

    struct msqid_ds queue_info;
    if (msgctl(msqid, IPC_STAT, &queue_info) == -1) {
        perror("msgctl IPC_STAT");
        return -1;
    }

    /*
     * msgsnd()'s msgsz excludes mtype but includes the message metadata.
     * Keep each message within both the kernel and this queue's limits.
     */
    size_t message_limit = kernel_limit;
    if ((size_t)queue_info.msg_qbytes < message_limit) {
        message_limit = (size_t)queue_info.msg_qbytes;
    }

    if (message_limit <= QUEUE_MESSAGE_METADATA_SIZE) {
        fprintf(stderr, "System V message limits are too small\n");
        return -1;
    }

    *max_payload_size = message_limit - QUEUE_MESSAGE_METADATA_SIZE;
    return 0;
}

static int sendMessage(int msqid, struct QueueMessage *message) {
    message->mtype = QUEUE_MESSAGE_TYPE;
    size_t message_size = QUEUE_MESSAGE_METADATA_SIZE + message->size;
    int result;

    do {
        result = msgsnd(msqid, message, message_size, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("msgsnd");
        return -1;
    }

    return 0;
}

static int sendChunk(int msqid, struct QueueMessage *message,
                     const char *data, size_t size, bool finished) {
    message->mtype = QUEUE_MESSAGE_TYPE;
    message->size = size;
    message->finished = finished;

    if (size > 0) {
        memcpy(message->data, data, size);
    }

    return sendMessage(msqid, message);
}

/*
 * A queue message cannot exceed the kernel's maximum message size.
 * Cap the application buffer before reading so each read produces at most
 * one data message and does not create a tiny remainder message.
 */
static int sendFile(int msqid, FILE *input, size_t buffer_size,
                    size_t max_payload_size) {
    if (buffer_size == 0 ||
        buffer_size > SIZE_MAX - offsetof(struct QueueMessage, data) ||
        max_payload_size > SIZE_MAX - offsetof(struct QueueMessage, data)) {
        fprintf(stderr, "Invalid buffer size\n");
        return -1;
    }

    size_t chunk_size = buffer_size;
    if (chunk_size > max_payload_size) {
        chunk_size = max_payload_size;
    }

    char *buffer = malloc(chunk_size);
    if (buffer == NULL) {
        perror("malloc input buffer");
        return -1;
    }

    size_t message_allocation_size =
        offsetof(struct QueueMessage, data) + chunk_size;
    struct QueueMessage *message = calloc(1, message_allocation_size);
    if (message == NULL) {
        perror("calloc message");
        free(buffer);
        return -1;
    }

    int result = 0;

    while (true) {
        size_t bytes_read = fread(buffer, 1, chunk_size, input);

        if (ferror(input)) {
            perror("fread");
            result = -1;
            break;
        }

        if (bytes_read > 0) {
            if (sendChunk(msqid, message, buffer, bytes_read, false) == -1) {
                result = -1;
                break;
            }
        }

        if (feof(input)) {
            break;
        }

        if (bytes_read == 0) {
            fprintf(stderr, "fread returned no data without EOF\n");
            result = -1;
            break;
        }
    }

    /* A separate terminator handles empty files and exact-size multiples. */
    if (result == 0) {
        result = sendChunk(msqid, message, NULL, 0, true);
    }

    free(message);
    free(buffer);
    return result;
}

static int parseBufferSize(const char *text, size_t *buffer_size) {
    if (text == NULL || *text == '\0') {
        fprintf(stderr, "Buffer size must be a positive integer\n");
        return -1;
    }

    for (const char *digit = text; *digit != '\0'; ++digit) {
        if (*digit < '0' || *digit > '9') {
            fprintf(stderr, "Invalid buffer size: %s\n", text);
            return -1;
        }
    }

    errno = 0;
    char *end = NULL;
    unsigned long long value = strtoull(text, &end, 10);

    if (errno == ERANGE || end == text || *end != '\0' ||
        value == 0 || value > SIZE_MAX) {
        fprintf(stderr, "Invalid buffer size: %s\n", text);
        return -1;
    }

    *buffer_size = (size_t)value;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <buffer_size>\n", argv[0]);
        return 1;
    }

    size_t buffer_size;
    if (parseBufferSize(argv[2], &buffer_size) == -1) {
        return 1;
    }

    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror("fopen input");
        return 1;
    }

    key_t queue_key = getIpcKey(IPC_KEY_FILE, QUEUE_PROJECT_ID);
    if (queue_key == (key_t)-1) {
        fclose(input);
        return 1;
    }

    int msqid = msgget(queue_key, IPC_CREAT | IPC_EXCL | 0600);
    if (msqid == -1) {
        perror("msgget");
        fclose(input);
        return 1;
    }

    size_t max_payload_size = 0;
    int result = getMaximumPayloadSize(msqid, &max_payload_size);
    if (result == 0) {
        result = sendFile(msqid, input, buffer_size, max_payload_size);
    }

    if (fclose(input) == EOF) {
        perror("fclose input");
        result = -1;
    }

    /* The receiver removes the queue after successful reception. */
    if (result == -1) {
        removeMessageQueue(msqid);
        return 1;
    }

    return 0;
}

