#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_ARGS 64
#define MAX_CMDS 32
#define INITIAL_BUFFER 1024

#define COLOR_PINK   "\033[38;5;205m"

char* command_reader();
int parse_args(char *cmd, char **args);
int split_pipeline(char *cmd, char *commands[]);
int run_one_command(char *cmd);
int run_pipeline(char *commands[], int cmd_count);

#endif