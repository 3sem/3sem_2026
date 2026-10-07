#ifndef PIPE_H
#define PIPE_H

void printProcessStatus(pid_t pid, const char *command, int status);

void seqPipe(char ***cmd);

#endif
