#include <stdio.h>

#include "duplex_channel.h"

size_t GetFileSize(int fd) {
    struct stat st = {0};
    if (fstat(fd, &st) == -1) {
        return 0;
    }
    return (size_t)st.st_size;
}
