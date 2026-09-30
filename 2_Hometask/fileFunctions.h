#ifndef FILE_FUNCTIONS_H
#define FILE_FUNCTIONS_H

const int INIT_FD = -1;

void closeFd(int* fd);

ssize_t readFull(int fd, char* buf, size_t size);

int writeFull(int fd, const char* buf, size_t size);

#endif
