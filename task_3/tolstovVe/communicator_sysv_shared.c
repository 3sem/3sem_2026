/* pipeline_sysv_shared.communicator */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <time.h>
#include "communicator.h"

#define SHM_SIZE 4096

/*
Случилось страшное: во время очередной перекомпиляции, уже на финальных этапах, этот
файл был каким-то непонтным образом полностью удалён без возможности восстановления.
Буквально в консоли подряд две команды: компиляция проходит как надо, потом сразу 
второй раз та же самая команда компиляции, но файла уже нет. Как я удалил этот файл, неизвестно.
К моему счастью, я заметил, что перед этим я успел скомпилировать готовый бинарник a.out,
где данный файл использовался в качестве исходника. Я сам в дизассемблировании не разбираюсь,
но зато отлично разбирается deepseek. Было достаточно описать, какие функции мне нужно вытащить,
и он моментально выдал мне почти точь-в-точь мой исходный код. Я, мягко говоря, обрадовался.
Весь этот код, технически, нейрослоп (отредактированный), но изначально написан мной.
Пусть этот коментарий лежит здесь в память об этом невероятном событии.
*/

int commInit(Communicator *communicator) {
    communicator->shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    if (communicator->shmid < 0) {
        perror("shmget failed");
        return -1;
    }

    communicator->shm = shmat(communicator->shmid, NULL, 0);
    if ((long int) communicator->shm == -1) {
        fprintf(stderr, "shmat failed %p %d\n", communicator->shm, errno);
        return -1;
    }

    communicator->sem = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    if (communicator->sem < 0) {
        perror("semget failed");
        return -1;
    }

	struct sembuf semb = {};
	semb.sem_num = 1;
	semb.sem_op = 1;
	semb.sem_flg = 0;
    if (semop(communicator->sem, &semb, 1) < 0) {
        perror("semop");
        return -1;
    }

    return 0;
}


int commDestroy(Communicator *communicator) {
    if (communicator->shm && communicator->shm != (void *)-1) {
        if (shmdt(communicator->shm) < 0) {
            perror("shmdt");
        }
        communicator->shm = NULL;
    }

    if (communicator->shmid >= 0) {
        if (shmctl(communicator->shmid, IPC_RMID, NULL) < 0) {
            perror("shmctl failed");
        }
    }

    if (communicator->sem >= 0) {
        semctl(communicator->sem, 0, IPC_RMID);
    }

    return 0;
}


int commWrite(Communicator *communicator, int pipeNumber, char *bits, size_t bufsize) {
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

    /* копируем данные в shared memory */
    int i = 0;
    for (; i < bufsize && i < SHM_SIZE - 1; i++) {
        communicator->shm[i] = bits[i];
    }
    for (int j = i; j < SHM_SIZE; j++) {
        communicator->shm[j] = 'x';
    }
    communicator->shm[i] = '\0';
    //fprintf(stderr, "/%d/", i);


    semb[0].sem_num = 1;//unlock sem 1
    semb[0].sem_op  = -1;
    semb[0].sem_flg = 0;
    if (semop(communicator->sem, semb, 1) < 0) {
        perror("semop1");
        return -1;
    }

    return i;
}


int commRead(Communicator *communicator, int pipeNumber, char *buf, size_t bufsize) {
    struct sembuf semb[2];

    semb[0].sem_num = 1;//wait by 1th semaphore
    semb[0].sem_op  = 0;
    semb[0].sem_flg = 0;
    semb[1].sem_num = 1;//block itself until enabling unblocking by the other
    semb[1].sem_op  = 1;
    semb[1].sem_flg = 0;
    if (semop(communicator->sem, semb, 2) < 0) {
        perror("semop3");
        return -1;
    }

    /* читаем из shared memory */
    int i = SHM_SIZE;

    int flag = 0;
    while (--i > 0 && communicator->shm[i] != '\0') {
        //fprintf(stderr, "%c", communicator->shm[i]);
        //flag = 1;
    }
    /*if (flag) {
        for (int j = 0; j < SHM_SIZE; j++) {
            fprintf(stderr, "|%d", communicator->shm[j]);
        }
    }*/
    //fprintf(stderr, "[%d]", i);
    memcpy(buf, communicator->shm, i);


    semb[0].sem_num = 0;//unlock sem 0
    semb[0].sem_op  = -1;
    semb[0].sem_flg = 0;
    if (semop(communicator->sem, semb, 1) < 0) {
        perror("semop4");
        return -1;
    }

    return i;
}