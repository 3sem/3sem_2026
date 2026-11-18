#include "common.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    size_t bufferSize = getBufferSize(argc, argv);
    if (!bufferSize)
        return EXIT_FAILURE;

    return transferFile(bufferSize) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
