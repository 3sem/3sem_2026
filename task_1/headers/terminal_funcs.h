#ifndef TERMINAL_FUNCS_H
#define TERMINAL_FUNCS_H

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    char **argv;
} Command;

char *trimWhitespace(char *text);
void freeCommands(Command *commands, size_t command_count);
int parseArguments(char *segment, char ***argv_out);
int parsePipeline(char *line, Command **commands_out, size_t *command_count_out);
void reportProcessStatus(pid_t pid, const char *command_name, int status);
int statusToExitCode(int status);
int waitForChild(pid_t pid, int *status_out);
int executePipeline(const Command *commands, size_t command_count);

#endif
