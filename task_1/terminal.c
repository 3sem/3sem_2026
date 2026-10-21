#define _POSIX_C_SOURCE 200809L

#include "terminal_funcs.h"

int main(void) {
    char *line = NULL;
    size_t line_capacity = 0;
    bool interactive = isatty(STDIN_FILENO);

    for (;;) {
        if (interactive) {
            fputs("terminal> ", stdout);
            fflush(stdout);
        }

        errno = 0;
        ssize_t bytes_read = getline(&line, &line_capacity, stdin);
        if (bytes_read == -1) {
            if (feof(stdin)) {
                break;
            }
            perror("getline");
            free(line);
            return EXIT_FAILURE;
        }

        if (bytes_read > 0 && line[bytes_read - 1] == '\n') {
            line[bytes_read - 1] = '\0';
        }

        char *input = trimWhitespace(line);
        if (*input == '\0') {
            continue;
        }

        if (strcmp(input, "exit") == 0) {
            break;
        }

        Command *commands = NULL;
        size_t command_count = 0;
        if (parsePipeline(input, &commands, &command_count) == -1) {
            continue;
        }

        (void)executePipeline(commands, command_count);
        freeCommands(commands, command_count);
    }

    free(line);
    return EXIT_SUCCESS;
}
