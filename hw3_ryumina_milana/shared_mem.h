#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "common.h"

typedef struct {
    size_t  buf_size;   
    bool    eof;        
    char    buffer[];   
} SharedData;

bool sem_wait   (int semid, int id);
bool sem_signal (int semid, int id);

#endif // SHARED_MEM_H