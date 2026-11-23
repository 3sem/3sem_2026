#include "common.h"
#include <sys/ipc.h>
#include <sys/msg.h>

static int queue = -1;
static struct Message {
    long type;
    unsigned char data[];
} *message;

static void cleanup(void)
{
    if (queue != -1) {
        msgctl(queue, IPC_RMID, NULL);
        queue = -1;
    }
}

static void send_message(size_t size)
{
    int result;
    do {
        result = msgsnd(queue, message, size, 0);
    } while (result == -1 && errno == EINTR);
    if (result == -1)
        fail("msgsnd");
}

static ssize_t receive_message(size_t size)
{
    ssize_t n;
    do {
        n = msgrcv(queue, message, size, 1, 0);
    } while (n == -1 && errno == EINTR);
    if (n == -1)
        fail("msgrcv");
    return n;
}

int main(int argc, char **argv)
{
    prepare(argc, argv);
    queue = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (queue == -1)
        fail("msgget");
    struct msqid_ds info;
    if (msgctl(queue, IPC_STAT, &info) == -1)
        fail("msgctl");
    size_t payload = block_size < info.msg_qbytes ? block_size : info.msg_qbytes;
    message = calloc(1, sizeof(*message) + payload);
    unsigned char *buffer = malloc(block_size);
    if (message == NULL || buffer == NULL)
        fail("malloc");
    message->type = 1;
    /* Пробуем сообщение в своей пустой очереди, не меняя системные лимиты. */
    while (msgsnd(queue, message, payload, IPC_NOWAIT) == -1) {
        if (errno == EINTR)
            continue;
        if ((errno != EINVAL && errno != EAGAIN) || payload <= 1)
            fail("message size probe");
        payload /= 2;
    }
    receive_message(payload);
    fprintf(stderr, "Message payload: %zu bytes\n", payload);

    double start = now();
    child = fork();
    if (child == -1)
        fail("fork");
    if (child == 0) {
        close(input);
        size_t used = 0;
        ssize_t n;
        while ((n = receive_message(payload)) > 0) {
            if ((size_t)n > block_size - used) {
                errno = EIO;
                fail("message boundary");
            }
            memcpy(buffer + used, message->data, (size_t)n);
            used += (size_t)n;
            if (used == block_size) {
                write_all(output, buffer, used);
                used = 0;
            }
        }
        write_all(output, buffer, used);
        if (close(output) == -1)
            fail("close");
        _exit(0);
    }
    close(output);
    ssize_t n;
    while ((n = read_block(input, buffer, block_size)) > 0) {
        for (size_t offset = 0; offset < (size_t)n;) {
            size_t part = (size_t)n - offset;
            if (part > payload)
                part = payload;
            memcpy(message->data, buffer + offset, part);
            send_message(part);
            offset += part;
        }
    }
    send_message(0); /* Пустое сообщение означает конец файла. */
    close(input);
    finish(start);
    free(buffer);
    free(message);
    return 0;
}
