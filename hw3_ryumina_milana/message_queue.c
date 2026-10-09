#include "common.h"

/* Максимальный размер сообщения System V MQ */
#define MSGMAX 8192

/* Типы сообщений */
#define MTYPE_DATA  1  
#define MTYPE_SIZE  2   
#define MTYPE_EOF   3   

struct MQMsg {
    long mtype;
    char mtext[MSGMAX];
};

int main(int argc, char **argv) {
    size_t buf_size = (argc > 1) ? (size_t)atol(argv[1]) : 8192;

    int fd_in = open("input.txt", O_RDONLY);
    if (fd_in == -1) {
        fprintf(stderr, "failed to open input.txt: %s\n", strerror(errno));
        return 1;
    }

    key_t mq_key = ftok("input.txt", 64);
    if (mq_key == -1) {
        fprintf(stderr, "ftok failed: %s\n", strerror(errno));
        close(fd_in);
        return 1;
    }

    int mqid = msgget(mq_key, IPC_CREAT | IPC_EXCL | 0660);
    if (mqid == -1) {
        mqid = msgget(mq_key, 0660);
        if (mqid == -1) {
            fprintf(stderr, "failed to create message queue: %s\n", strerror(errno));
            close(fd_in);
            return 1;
        }
    }

    struct MQMsg msg;
    msg.mtype = MTYPE_DATA;

    /* Буфер для чтения из файла (размер = buf_size) */
    char *buffer = malloc(buf_size);
    if (buffer == NULL) {
        fprintf(stderr, "failed to allocate buffer\n");
        msgctl(mqid, IPC_RMID, NULL);
        close(fd_in);
        return 1;
    }

    struct timespec start = {}, end = {};

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "failed to fork: %s\n", strerror(errno));
        free(buffer);
        msgctl(mqid, IPC_RMID, NULL);
        close(fd_in);
        return 1;
    }

    if (pid == 0) {
        /* ---------- Ребёнок: принимает*/
        int fd_out = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1) {
            fprintf(stderr, "failed to open output.txt: %s\n", strerror(errno));
            free(buffer);
            exit(1);
        }

        while (1) {
            ssize_t n = msgrcv(mqid, &msg, sizeof(size_t), MTYPE_SIZE, 0);
            if (n == -1) {
                fprintf(stderr, "msgrcv (size) failed: %s\n", strerror(errno));
                close(fd_out);
                free(buffer);
                exit(1);
            }

            size_t chunk_size;
            memcpy(&chunk_size, msg.mtext, sizeof(size_t));

            if (chunk_size == 0) {
                break;
            }

            /* Принимаем данные порции кусками ≤ MSGMAX */
            size_t received = 0;
            while (received < chunk_size) {
                ssize_t r = msgrcv(mqid, &msg, MSGMAX, MTYPE_DATA, 0);
                if (r == -1) {
                    fprintf(stderr, "msgrcv (data) failed: %s\n", strerror(errno));
                    close(fd_out);
                    free(buffer);
                    exit(1);
                }
                memcpy(buffer + received, msg.mtext, (size_t)r);
                received += (size_t)r;
            }

            size_t written = 0;
            while (written < chunk_size) {
                ssize_t w = write(fd_out, buffer + written, chunk_size - written);
                if (w <= 0) {
                    fprintf(stderr, "write failed: %s\n", strerror(errno));
                    close(fd_out);
                    free(buffer);
                    exit(1);
                }
                written += (size_t)w;
            }
        }

        close(fd_out);
        free(buffer);
        exit(0);
    } else {
        /* ---------- Родитель: отправляет */
        clock_gettime(CLOCK_MONOTONIC, &start);

        ssize_t n;
        while ((n = read(fd_in, buffer, buf_size)) > 0) {
            size_t to_send = (size_t)n;

            msg.mtype = MTYPE_SIZE;
            memcpy(msg.mtext, &to_send, sizeof(size_t));
            if (msgsnd(mqid, &msg, sizeof(size_t), 0) == -1) {
                fprintf(stderr, "msgsnd (size) failed: %s\n", strerror(errno));
                break;
            }

            /* Отправляем данные кусками ≤ MSGMAX */
            size_t offset = 0;
            while (offset < to_send) {
                size_t chunk = to_send - offset;
                if (chunk > MSGMAX) {
                    chunk = MSGMAX;
                }

                msg.mtype = MTYPE_DATA;
                memcpy(msg.mtext, buffer + offset, chunk);
                if (msgsnd(mqid, &msg, chunk, 0) == -1) {
                    fprintf(stderr, "msgsnd (data) failed: %s\n", strerror(errno));
                    break;
                }
                offset += chunk;
            }
        }

        if (n == -1) {
            fprintf(stderr, "read failed: %s\n", strerror(errno));
        }

        /* Отправляем EOF: заголовок с нулём */
        size_t zero = 0;
        msg.mtype = MTYPE_SIZE;
        memcpy(msg.mtext, &zero, sizeof(size_t));
        if (msgsnd(mqid, &msg, sizeof(size_t), 0) == -1) {
            fprintf(stderr, "msgsnd (EOF) failed: %s\n", strerror(errno));
        }

        close(fd_in);
        wait(NULL);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (double)(end.tv_sec - start.tv_sec) * 1e9;
        time_taken = (time_taken + (double)(end.tv_nsec - start.tv_nsec)) * 1e-9;

        printf("Time duration: %lg\n", time_taken);

        free(buffer);
        msgctl(mqid, IPC_RMID, NULL);
    }

    return 0;
}