#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "duplex_channel.h"

/* ---------- Приватные методы ---------- */

/* Прочитать из direct-pipe (родитель→ребёнок читает ребёнок,
   ребёнок→родитель читает родитель) */
static ssize_t ReadDuplex(DuplexPipe *self) {
    assert(self);
    return read(self->fd_direct[0], self->data, self->data_size);
}

/* Записать в back-pipe (ребёнок пишет родителю) */
static ssize_t WriteDuplex(DuplexPipe *self) {
    assert(self);
    return write(self->fd_back[1], self->data, self->len);
}

/* Закрыть концы, ненужные родителю */
static void CloseParentPipes(DuplexPipe *self) {
    assert(self);
    close(self->fd_direct[0]);
    close(self->fd_back[1]);
}

/* Закрыть концы, ненужные ребёнку */
static void CloseChildPipes(DuplexPipe *self) {
    assert(self);
    close(self->fd_direct[1]);
    close(self->fd_back[0]);
}

/* Закрыть всё (для завершения) */
static void CloseAllPipes(DuplexPipe *self) {
    assert(self);
    close(self->fd_direct[0]);
    close(self->fd_direct[1]);
    close(self->fd_back[0]);
    close(self->fd_back[1]);
}

/* ---------- Основной сценарий ---------- */

void Run(DuplexPipe *self) {
    assert(self);

    pid_t pid = -1;

    struct timespec start = {}, end = {};
    clock_gettime(CLOCK_MONOTONIC, &start);

    if ((pid = fork()) == -1) {
        fprintf(stderr, "failed to create new process\n");
        return;
    } else if (pid == 0) {
        /* ---------- Ребёнок ---------- */
        /* Закрываем концы, которые ребёнку не нужны */
        self->actions.close_child(self);

        ssize_t n;
        /* Читаем из direct, пишем в back — эхо */
        while ((n = self->actions.rcv(self)) > 0) {
            self->len = (size_t)n;
            self->actions.snd(self);
        }

        self->actions.close_all(self);
        exit(0);
    } else {
        /* ---------- Родитель ---------- */
        self->actions.close_parent(self);

        int in = open("parent.txt", O_RDONLY);
        if (in == -1) {
            fprintf(stderr, "failed to open parent file\n");
            return;
        }

        int out = open("child.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out == -1) {
            fprintf(stderr, "failed to open child file\n");
            close(in);
            return;
        }

        ssize_t len = 0;
        /* Цикл: читаем файл, отправляем, принимаем эхо, пишем в out */
        while ((len = read(in, self->data, self->data_size)) > 0) {
            self->len = (size_t)len;

            /* Отправить ребёнку */
            if (self->actions.snd(self) != len) break;

            /* Принять от ребёнка */
            if (self->actions.rcv(self) != len) break;

            /* Записать в выходной файл */
           ssize_t written = write(out, self->data, (size_t)len);
	   if (written != len) {
  		perror("write to output");
		break;
	   }
        }

        self->actions.close_all(self);
        close(in);
        close(out);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            printf("Program exited with code %d\n", exit_code);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (double)(end.tv_sec - start.tv_sec) * 1e9;
    time_taken = (time_taken + (double)(end.tv_nsec - start.tv_nsec)) * 1e-9;
    printf("Time duration: %lg s\n", time_taken);
}

/* ---------- Конструктор и деструктор ---------- */

DuplexPipe* CreateDuplexPipe(size_t buffer_size) {
    DuplexPipe* self = (DuplexPipe*)calloc(1, sizeof(DuplexPipe));
    if (self == NULL) {
        fprintf(stderr, "failed to allocate memory for Pipe\n");
        return NULL;
    }

    self->data = (char*)calloc(buffer_size, sizeof(char));
    if (self->data == NULL) {
        fprintf(stderr, "failed to allocate memory for data\n");
        free(self);
        return NULL;
    }
    self->data_size = buffer_size;

    if (pipe(self->fd_direct) == -1 || pipe(self->fd_back) == -1) {
        fprintf(stderr, "failed to initialize pipe\n");
        free(self->data);
        free(self);
        return NULL;
    }

    self->actions.rcv = ReadDuplex;
    self->actions.snd = WriteDuplex;
    self->actions.close_child = CloseChildPipes;
    self->actions.close_parent = CloseParentPipes;
    self->actions.close_all = CloseAllPipes;

    return self;
}

void DestroyDuplexPipe(DuplexPipe *self) {
    if (self == NULL) return;

    close(self->fd_direct[0]);
    close(self->fd_direct[1]);
    close(self->fd_back[0]);
    close(self->fd_back[1]);

    free(self->data);
    free(self);
}
