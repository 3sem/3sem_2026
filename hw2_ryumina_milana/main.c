#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "file.h"
#include "duplex_channel.h"

int main(int argc, char **argv) {
    size_t size_kb = 64;   /* 64 КБ, если не придет из ран другой размер */

    if (argc > 1) {
        char *end = NULL;
        errno = 0;
        unsigned long val = strtoul(argv[1], &end, 10);
        if (errno != 0 || *end != '\0' || val == 0) {
            fprintf(stderr, "usage: %s [buffer_size_kb]\n", argv[0]);
            return 1;
        }
        size_kb = (size_t)val;
    }

    size_t buffer_size = size_kb * 1024;

    DuplexPipe *pipe = CreateDuplexPipe(buffer_size);
    if (pipe == NULL) {
        fprintf(stderr, "failed to create DuplexPipe\n");
        return 1;
    }

    Run(pipe);
    DestroyDuplexPipe(pipe);
    return 0;
}
