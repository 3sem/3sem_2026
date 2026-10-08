#include "modes.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    if (argc > 1 && strcmp(argv[1], "send") == 0)
    {
        return sendMode(argc - 1, argv + 1);
    }

    if (argc == 2 && strcmp(argv[1], "read") == 0)
    {
        return readMode();
    }

    fprintf(stderr, "Usage: %s send EXPRESSION N_THREADS\n"
                    "       %s read\n", argv[0], argv[0]);
    return 1;
}
