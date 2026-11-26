#include "sig.h"

static const int kSigBufSize   = 4;
static const int kSigBlockSize = 128;

void start_sender(signal_t *sig, sem_t *sem, buffer_t *buffer){
    int input_fd = open(source, O_RDONLY | O_CREAT, 0644);

    while (buffer->actions->write_in_buffer(buffer, input_fd)){
        char *current_pos = buffer->buffer;

        while (buffer->size != 0){
            sem->sem_ops->wait_writer(sem);

            for (int sig_num = 0; sig_num < kSigBlockSize && buffer->size != 0; sig_num++){
                if (buffer->size < kSigBufSize){
                    send_sig_data(sig, current_pos, buffer->size);
                    buffer->size = 0;
                    break;
                }
                else {
                    send_sig_data(sig, current_pos, kSigBufSize);
                    buffer->size -= kSigBufSize;
                    current_pos  += kSigBufSize;
                }
            }
            sem->sem_ops->open_reader(sem);
        }
    }

    sem->sem_ops->open_reader(sem);
    sigqueue(sig->dest, sig->end_sig, (union sigval){0});
    close(input_fd);
}

void start_receiver(signal_t *sig, sem_t *sem, buffer_t *buffer){
    int output_fd = open(destination, O_WRONLY | O_CREAT, 0644);
    
    int finished     = 0;
    buffer->size     = 0;
    int sig_buf_size = 0;

    siginfo_t info;

    while (!finished){
        sem->sem_ops->wait_reader(sem);

        for (int sig_num = 0; sig_num < kSigBlockSize; sig_num++){ //TODO size
            int received_sig;

            do {
                received_sig = sigwaitinfo(sig->mask, &info);
            } while (received_sig == -1 && errno == EINTR);

            if (received_sig == sig->end_sig){
                finished = 1;
                break;
            }
            
            if      (received_sig == sig->buf_size_1) sig_buf_size = 1;
            else if (received_sig == sig->buf_size_2) sig_buf_size = 2;
            else if (received_sig == sig->buf_size_3) sig_buf_size = 3;
            else if (received_sig == sig->buf_size_4) sig_buf_size = 4;

            if (buffer->size + sig_buf_size > buffer->max_size) {
                buffer->actions->read_from_buffer(buffer, output_fd);
                buffer->size = 0;
            }

            memcpy(buffer->buffer + buffer->size,  &info.si_value, sig_buf_size);
            buffer->size += sig_buf_size;
        }

        sem->sem_ops->open_writer(sem);
    }

    if (buffer->buffer > 0){
        buffer->actions->read_from_buffer(buffer, output_fd);
    }
    close(output_fd);
}


void send_sig_data(signal_t *sig, void *data, int size){
    union sigval sv;

    memcpy(&sv, data, size);
    switch (size){
        case 1:
            sigqueue(sig->dest, sig->buf_size_1, sv);
            break;
        case 2:
            sigqueue(sig->dest, sig->buf_size_2, sv);
            break;
        case 3:
            sigqueue(sig->dest, sig->buf_size_3, sv);
            break;
        case 4:
            sigqueue(sig->dest, sig->buf_size_4, sv);
            break;
        default:
            perror("data size error\n");
            break;
    }
}

signal_t *create_sig(pid_t dest_pid){
    signal_t *new_sig = malloc(sizeof(signal_t));

    new_sig->dest    = dest_pid;

    new_sig->end_sig    = SIGRTMIN + 4;
    new_sig->buf_size_1 = SIGRTMIN + 3;
    new_sig->buf_size_2 = SIGRTMIN + 2;
    new_sig->buf_size_3 = SIGRTMIN + 1;
    new_sig->buf_size_4 = SIGRTMIN;

    new_sig->mask = malloc(sizeof(sigset_t));

    sigemptyset(new_sig->mask);
    sigaddset(new_sig->mask, new_sig->buf_size_1);
    sigaddset(new_sig->mask, new_sig->buf_size_2);
    sigaddset(new_sig->mask, new_sig->buf_size_3);
    sigaddset(new_sig->mask, new_sig->buf_size_4);
    sigaddset(new_sig->mask, new_sig->end_sig   );

    // Блокируем их, чтобы ядро складывало сигналы в очередь для sigwaitinfo
    if (sigprocmask(SIG_BLOCK, new_sig->mask, NULL) == -1) {
        perror("sigprocmask error");
    }

    new_sig->sig_ops = default_sig_ops;
    
    return new_sig;
}

void destroy_sig(signal_t *sig){
    free(sig);
    free(sig->mask);
}