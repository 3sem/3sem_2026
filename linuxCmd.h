#ifndef LINUX_CMD_H
#define LINUX_CMD_H

#include <sys/types.h>

int createPipe( int* fd);
int dupFd2( int dst, int src);
int execFromPipe( const char* fileName, char* const* args);

pid_t forkProc();

#endif /* LINUX_CMD_H */