#ifndef DUPLEX_H
#define DUPLEX_H

#include <stddef.h>
#include <sys/types.h>

typedef struct Duplex Duplex;
typedef struct Ops Ops;
typedef struct Pair Pair;


struct Ops {
    ssize_t (*Send)(Duplex* self, const void* buf, size_t len);
    ssize_t (*Recv)(Duplex* self, void* buf, size_t len);

    ssize_t (*SendFile)(Duplex* self, int in_fd, size_t len);
    ssize_t (*RecvFile)(Duplex* self, int out_fd, size_t len);

    ssize_t (*Echo)(Duplex* self, size_t len);
};

struct Duplex {
    int in;       
    int out;         
    size_t pipe_size;
    Ops op_table;
};

struct Pair {
    Duplex* duplex_1; 
    Duplex* duplex_2;   
};

Pair Construct(size_t pipe_size);

void Destruct(Duplex* self);

#endif /* DUPLEX_H */