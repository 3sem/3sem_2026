#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

#include "duplex.h"

int main (void) {
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("Failed signal.");
        return EXIT_FAILURE;
    }

    duplex_t* duplex = duplexCtor();
    if (!duplex) {
        return EXIT_FAILURE;
    }

    int isChild = duplex->pid == 0;
    int result = isChild ? childEcho(duplex->child) : parentEcho(duplex->parent);

    duplexDtor(duplex);

    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

