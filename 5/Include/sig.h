#ifndef SIG_H_
#define SIG_H_

#include "buffer.h"
#include "sem.h"
#include <string.h>
#include <errno.h>

typedef struct sig signal_t;

typedef struct {
    void (*send_sig_data)(signal_t* , void*, int);
} sig_ops_t;

struct sig {
    pid_t dest;

    int buf_size_1;
    int buf_size_2;
    int buf_size_3;
    int buf_size_4;
    int end_sig;

    sigset_t *mask;
    sig_ops_t sig_ops;
};

void send_sig_data(signal_t *sig, void *data, int size);

static const sig_ops_t default_sig_ops = {
    .send_sig_data = send_sig_data,
};

signal_t *create_sig (pid_t dest_pid);
void      destroy_sig(signal_t *sig);

void start_sender  (signal_t *sig, sem_t *sem, buffer_t *buffer);
void start_receiver(signal_t *sig, sem_t *sem, buffer_t *buffer);

#endif