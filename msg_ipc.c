// msg_ipc.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <string.h>
#include <sys/msg.h>
#include <time.h>
#include <sys/wait.h>

#define MSG_KEY 0x2345
#define MAX_MSG_CHUNK 4096 // Системное ограничение для очереди

struct msgbuf {
    long mtype;
    char mtext[MAX_MSG_CHUNK];
};

int main(int argc, char *argv[]) {
    if (argc < 4) return 1;
    char *file_in = argv[1];
    char *file_out = argv[2];
    size_t buf_size = atoi(argv[3]); 

    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);

    pid_t pid = fork();
    if (pid == 0) { // Потомок - Приемник
        FILE *out = fopen(file_out, "wb");
        struct msgbuf buf;
        
        while (1) {
            ssize_t res = msgrcv(msgid, &buf, MAX_MSG_CHUNK, 1, 0);
            if (res < 0) break;
            if (res == 0) break; // Сигнал EOF
            fwrite(buf.mtext, 1, res, out);
        }
        fclose(out);
        return 0;
    } else { // Родитель - Источник
        FILE *in = fopen(file_in, "rb");
        char *temp_buf = malloc(buf_size);
        struct msgbuf msg;
        msg.mtype = 1;

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        while (1) {
            size_t read_bytes = fread(temp_buf, 1, buf_size, in);
            if (read_bytes == 0) {
                msgsnd(msgid, &msg, 0, 0); // Посылаем пустой маркер EOF
                break;
            }
            // Дробим наш логический буфер на куски по MAX_MSG_CHUNK, если он больше
            size_t sent = 0;
            while (sent < read_bytes) {
                size_t to_send = (read_bytes - sent > MAX_MSG_CHUNK) ? MAX_MSG_CHUNK : (read_bytes - sent);
                memcpy(msg.mtext, temp_buf + sent, to_send);
                msgsnd(msgid, &msg, to_send, 0);
                sent += to_send;
            }
        }
        free(temp_buf);
        fclose(in);
        wait(NULL);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("MSG Queue Time (%zu bytes): %.4f sec\n", buf_size, elapsed);
        msgctl(msgid, IPC_RMID, NULL);
    }
    return 0;
}
