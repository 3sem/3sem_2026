#include <signal.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "communicator.h"


#define BUFSIZE 4096
#define COMM_SIGNAL

typedef union {
	int int_val;
	char char_val[4];
	short two_shorts[2];
} char_to_int;


static char commBuffer[BUFSIZE] = {};
//static int kostil = 1;
static char *commBuffPointer = commBuffer;
static int sem_duplicate = 0;
static int send_counter;

static short conservation_of_data[];

void handler(int sig, siginfo_t *info, void *ucontext) {
	char_to_int c = {.int_val = info->si_value.sival_int};
    //fprintf(stderr, "Transmitted: %c%c%c%c = %d\n", c.char_val[0], c.char_val[1], c.char_val[2], c.char_val[3], c.int_val);  // Read integer from sival

	if (commBuffPointer - &commBuffer[0] > BUFSIZE - 4) {
		fprintf(stderr, "buffer overflow");
		return;
	}
	*(commBuffPointer + 0) = c.char_val[0];
	*(commBuffPointer + 1) = c.char_val[1];
	*(commBuffPointer + 2) = c.char_val[2];
	*(commBuffPointer + 3) = c.char_val[3];
	commBuffPointer += 4;
}


void sendrt(int pid, int signal, char* buf) {
	char_to_int c = {.char_val = {buf[0], buf[1], buf[2], buf[3]}};
	union sigval sv = {.sival_int = c.int_val};
	sigqueue(pid, signal, sv);
	//printf("sending signal %d to pid %d with data [%c|%c|%c|%c] = %d\n", signal, pid, buf[0], buf[1], buf[2], buf[3], sv.sival_int);
}


int get_sent_amount() {
	return send_counter;
}


int commInit(Communicator *communicator) {
	communicator->shm = NULL;

	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;
	if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("Ошибка sigaction");
		return -1;
    }

	sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGRTMIN + 1);
    sigprocmask(SIG_BLOCK, &set, NULL);  // Block signal
	
    communicator->sem = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
	sem_duplicate = communicator->sem;
    if (communicator->sem < 0) {
        perror("semget failed");
        return -1;
    }

    return 0;
}


int commDestroy(Communicator *communicator) {
    if (communicator->sem >= 0) {
        semctl(communicator->sem, 0, IPC_RMID);
    }

    return 0;
}


int commWrite(Communicator *communicator, int pipeNumber, char *bits, size_t bufsize) {
	
	/*if (kostil) {
		sigset_t set;
		sigemptyset(&set);
		sigaddset(&set, SIGRTMIN + 1);
		sigaddset(&set, SIGRTMIN);
		sigprocmask(SIG_BLOCK, &set, NULL);  // Block signal
		kostil = 0;
	}*/

    struct sembuf semb[2];
    semb[0].sem_num = 0;//wait by 0th semaphore
    semb[0].sem_op  = 0;
    semb[0].sem_flg = 0;
    semb[1].sem_num = 0;//block itself until enabling unblocking by the other
    semb[1].sem_op  = 1;
    semb[1].sem_flg = 0;


    if (semop(communicator->sem, semb, 2) < 0) {
        perror("semop1");
        return -1;
    }
	
    int i = 0;
    for (; i < (long int) bufsize - 3 && i < (long int) BUFSIZE - 4; i += 4) {
		sendrt(communicator->shmid, SIGRTMIN, bits + i);
		/*if (usleep(100000) < 0) {
			perror("what the hell");
		}*/
    }
	char buf2[4] = "xxxx";
	int j = i;
	for (; j < bufsize && j < (long int) BUFSIZE - 1; j++) {
		buf2[j-i] = bits[j];
	}
	buf2[j-i] = '\0';
	sendrt(communicator->shmid, SIGRTMIN, buf2);
	char_to_int c = {.int_val = j};

	c.two_shorts[1] = send_counter++;
	//fprintf(stderr, "sent %d bytes №%d\n", j, c.two_shorts[1]);
	sendrt(communicator->shmid, SIGRTMIN + 1, c.char_val);

    return j;
}


int commRead(Communicator *communicator, int pipeNumber, char *buf, size_t bufsize) {
	sigset_t set;
	siginfo_t info;
    sigemptyset(&set);
    sigaddset(&set, SIGRTMIN + 1);
	
	//fprintf(stderr, "waiting for signal %d\n", SIGRTMIN + 1);

	sigset_t blocked_set;
    if (sigprocmask(SIG_BLOCK, NULL, &blocked_set) == -1) {
        perror("Failed to fetch sigprocmask");
        return -1;
    }
    /*printf("Blocked signals:\n");
	if (sigismember(&blocked_set, SIGRTMIN)) {
		fprintf(stderr, "SIGRTMIN blocked\n");
	}
	if (sigismember(&blocked_set, SIGRTMIN + 1)) {
		fprintf(stderr, "SIGRTMIN + 1 blocked\n");
	}*/

	int sig;
	while ((sig = sigwaitinfo(&set, &info)) < 0) {
		if (errno == EINTR) continue;
		perror("sigwaitinfo failed");
		return -1;
	}
	char_to_int c = {.int_val = info.si_value.sival_int};
	//fprintf(stderr, "received signal %d №%d with data %d\n", SIGRTMIN + 1, c.two_shorts[1], c.two_shorts[0]);
	
	memcpy(buf, commBuffer, c.two_shorts[0]);
	commBuffPointer = &commBuffer[0];
	send_counter++;
	//fprintf(stderr, "text: %d\n", c.two_shorts[0]);

	struct sembuf semb[1];
    semb[0].sem_num = 0;//unlock sem 0
    semb[0].sem_op  = -1;
    semb[0].sem_flg = 0;
	
    /*sigemptyset(&set);
    sigaddset(&set, SIGRTMIN);
    sigprocmask(SIG_BLOCK, &set, NULL);  // Block signal*/
    if (semop(communicator->sem, semb, 1) < 0) {
        perror("semop4");
        return -1;
    }
    //sigprocmask(SIG_UNBLOCK, &set, NULL);  // unblock signal

	//perror("finished receiving");
    return (int) c.two_shorts[0];
}
