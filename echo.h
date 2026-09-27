#ifndef ECHO_H
#define ECHO_H

#include "duplex_pipe.h"

typedef enum { ECHO_MODE_SPLICE, ECHO_MODE_COPY } EchoMode;

int echo_child(DuplexPipe *dp, EchoMode mode);

int echo_parent(DuplexPipe *dp, const char *src, const char *dst,
                EchoMode mode);

#endif
