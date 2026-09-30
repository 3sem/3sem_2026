#ifndef DUPLEX_H
#define DUPLEX_H

#include "process.h"

const size_t BUF_SIZE = 64 * 1024;

typedef struct duplex_t {
    process_t* parent;
    process_t* child;
    pid_t pid;
} duplex_t;

duplex_t* duplexCtor ();

void duplexDtor (duplex_t* duplex);

int childEcho(process_t* child);

int parentEcho(process_t* parent);

#endif
