#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include "execute.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    char *line = NULL;
    size_t size = 0;

    while (1) {
        if (isatty(STDIN_FILENO)) {
            printf("shell> ");
            fflush(stdout);
        }

        if (getline(&line, &size, stdin) == -1)
            break;

        Pipeline p;
        int result = parse_line(line, &p);

        if (result == 1) {
            run_commands(&p);
            free_line(&p);
        }
    }

    free(line);
    return 0;
}
