#include <sig.h>

int main(){
    pid_t     pid;
    int       status;

    buffer_t *buffer;
    sem_t    *sem;
    signal_t *sig;

    if (!(sem    = create_sem   ()))      return 0;
    if (!(buffer = create_buffer()))      return 0;
    if (!(sig    = create_sig(getpid()))) return 0;


    pid = fork();

    switch (pid){
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;

        case 0: 
            start_sender(sig, sem, buffer);
            break;

        default:
            start_receiver(sig, sem, buffer);
            waitpid(pid, NULL, 0);
            break;
    }

    if (pid != 0) {
        destroy_sem(sem);}
    else {
        free(sem);
    }
    destroy_buffer(buffer);
    destroy_sig   (sig);
}

