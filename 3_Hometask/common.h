#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <sys/types.h>

size_t getBufferSize(int argc, char* argv[]);

ssize_t readChunk(int fd, void* buffer, size_t bufferSize);

int writeFull(int fd, const void* buffer, size_t dataSize);

int copyFile(int inputFd, int outputFd, size_t bufferSize);

void closeFd(int* fd);

int waitChild(pid_t childPid);

int transferFile(size_t bufferSize);

#endif
