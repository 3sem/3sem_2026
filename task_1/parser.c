#define _POSIX_C_SOURCE 200809L
#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_line(const char *line, Pipeline *p)
{
    int len = (int)strlen(line);
    int pipes = 0;

    for (int i = 0; i < len; i++) {
        if (line[i] == '|')
            pipes++;
    }

    p->line = strdup(line);
    p->commands = calloc((size_t)pipes + 1, sizeof(Command));
    p->args = calloc((size_t)len + 2, sizeof(char *));
    p->count = 0;

    if (p->line == NULL || p->commands == NULL || p->args == NULL) {
        free_line(p);
        perror("memory");
        return -1;
    }

    char *read = p->line;
    char *write = p->line;
    char *start = NULL;
    char quote = 0;
    int started = 0;
    int arg = 0;

    p->commands[0].argv = p->args;

    while (1) {
        char c = *read;

        if (quote != 0) {
            if (c == '\0') {
                fprintf(stderr, "missing quote\n");
                free_line(p);
                return -1;
            }
            if (c == quote) {
                quote = 0;
            } else {
                if (!started) {
                    start = write;
                    started = 1;
                }
                *write++ = c;
            }
            read++;
            continue;
        }

        if (c == '\'' || c == '"') {
            if (!started) {
                start = write;
                started = 1;
            }
            quote = c;
            read++;
            continue;
        }

        if (c == '|' || c == '\0' || isspace((unsigned char)c)) {
            if (started) {
                *write++ = '\0';
                p->args[arg++] = start;
                started = 0;
            }

            if (c == '|') {
                if (p->commands[p->count].argv == NULL ||
                    p->commands[p->count].argv[0] == NULL) {
                    fprintf(stderr, "empty command\n");
                    free_line(p);
                    return -1;
                }
                p->args[arg++] = NULL;
                p->count++;
                p->commands[p->count].argv = &p->args[arg];
            } else if (c == '\0') {
                if (p->commands[p->count].argv[0] != NULL) {
                    p->args[arg] = NULL;
                    p->count++;
                } else if (p->count != 0) {
                    fprintf(stderr, "command after pipe is missing\n");
                    free_line(p);
                    return -1;
                }
                break;
            }

            if (c != '\0')
                read++;
            continue;
        }

        if (!started) {
            start = write;
            started = 1;
        }
        *write++ = c;
        read++;
    }

    if (p->count == 0) {
        free_line(p);
        return 0;
    }

    return 1;
}

void free_line(Pipeline *p)
{
    free(p->line);
    free(p->commands);
    free(p->args);
    p->line = NULL;
    p->commands = NULL;
    p->args = NULL;
    p->count = 0;
}
