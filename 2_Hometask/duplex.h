#ifndef DUPLEX_H
#define DUPLEX_H

#include <stddef.h>
#include <sys/types.h>

static const size_t BUF_SIZE = 64 * 1024;

typedef struct duplex_t duplex_t;

typedef struct duplex_ops_t {
    ssize_t (*read)(duplex_t *self, char *buf, size_t size);
    int (*write)(duplex_t *self, const char *buf, size_t size);
    void (*finishWrite)(duplex_t *self);
} duplex_ops_t;

struct duplex_t {
    int toChild[2];
    int toParent[2];
    pid_t pid;
    duplex_ops_t ops;
};

duplex_t* duplexCtor ();

void duplexDtor(duplex_t* duplex);

ssize_t duplexRead(duplex_t* duplex, char* buf, size_t size);

int duplexWrite(duplex_t* duplex, const char* buf, size_t size);

void duplexFinishWrite(duplex_t* duplex);

int duplexExchange(duplex_t* duplex, char* buf, size_t size, int lastBlock);

int childEcho(duplex_t* duplex);

int parentEcho(duplex_t* duplex);

#endif
