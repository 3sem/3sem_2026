#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct {
    long mtype;
    size_t bytes;
    char data[];
} msg_buf_t;

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Использование: %s <input> <output> <buf_size_bytes>\n", argv[0]);
        return 1;
    }

    char* input_file = argv[1];
    char* output_file = argv[2];
    size_t buf_size = atoll(argv[3]);

    int in_fd = open(input_file, O_RDONLY);
    if (in_fd < 0) {
        perror("open input");
        return 1;
    }

    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd < 0) {
        perror("open output");
        close(in_fd);
        return 1;
    }

    int msqid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msqid < 0) {
        perror("msgget");
        return 1;
    }

    size_t msg_struct_size = sizeof(msg_buf_t) + buf_size;
    msg_buf_t* msg = (msg_buf_t*) malloc(msg_struct_size);
    if (!msg) {
        perror("malloc");
        msgctl(msqid, IPC_RMID, NULL);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(msg);
        msgctl(msqid, IPC_RMID, NULL);
        return 1;
    }

    if (pid == 0) { //child process
        close(in_fd);

        while (1) {
            ssize_t payload_size = sizeof(size_t) + buf_size;
            if (msgrcv(msqid, msg, payload_size, 1, 0) < 0) {
                perror("msgrcv");
                exit(1);
            }

            if (msg->bytes == 0) {
                break;
            }

            write(out_fd, msg->data, msg->bytes);
        }

        close(out_fd);
        free(msg);
        exit(0);
    }
    else { //parent process
        close(out_fd);

        msg->mtype = 1;

        int failed = 0;
        while (1) {
            ssize_t bytes_read = read(in_fd, msg->data, buf_size);
            if (bytes_read < 0) {
                perror("read input");
                failed = 1;
                break;
            }

            msg->bytes = (size_t)bytes_read;

            size_t send_payload_size = sizeof(size_t) + (bytes_read > 0 ? bytes_read : 0);
            if (msgsnd(msqid, msg, send_payload_size, 0) < 0) {
                perror("msgsnd");
                failed = 1;
                break;
            }

            if (bytes_read == 0) {
                break;
            }
        }

        close(in_fd);

        if (failed) {
            msgctl(msqid, IPC_RMID, NULL);
        }

        wait(NULL);

        if (!failed) {
            msgctl(msqid, IPC_RMID, NULL);
        }

        free(msg);
    }

    return 0;

}
