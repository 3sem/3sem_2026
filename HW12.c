#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void process_escapes(char *s) {
    char *read = s;
    char *write = s;
    while (*read) {
        if (*read == '\\' && *(read + 1) != '\0') {
            read++;
            if (*read == ' ') {
                *write++ = 0x01;
            } else {
                *write++ = *read;
            }
            read++;
        } else {
            *write++ = *read++;
        }
    }
    *write = '\0';
}

char ***parse(const char *line) {
    char *copy = strdup(line);
    char ***cmds = malloc(sizeof(char**) * 16);
    int ci = 0;

    char *save1, *save2;
    char *cmd_str = strtok_r(copy, "|", &save1);

    while (cmd_str != NULL && ci < 16 - 1) {
        process_escapes(cmd_str);

        char **argv = malloc(sizeof(char*) * 32);
        int ai = 0;

        char *word = strtok_r(cmd_str, " \t\n", &save2);
        while (word != NULL && ai < 32 - 1) {
            argv[ai++] = word;
            word = strtok_r(NULL, " \t\n", &save2);
        }
        argv[ai] = NULL;

        int i;
        for (i = 0; i < ai; i++) {
            char *p;
            for (p = argv[i]; *p; p++) {
                if (*p == 0x01) *p = ' ';
            }
        }

        if (ai > 0) cmds[ci++] = argv;
        cmd_str = strtok_r(NULL, "|", &save1);
    }
    cmds[ci] = NULL;
    return cmds;
}

void seq_pipe(char ***cmds) {
    int fd_in = 0;
    int p[2];
    int i = 0;

    while (cmds[i] != NULL) {
        if (pipe(p) == -1){
            perror("pipe");
            exit(1);
        }
        pid_t pid = fork();
        if (pid == -1) { 
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            if (i > 0){
                dup2(fd_in, STDIN_FILENO);
            } 
            if (cmds[i+1] != NULL){
                dup2(p[1], STDOUT_FILENO);
            }
            close(p[0]);
            close(p[1]);
            if (i > 0){
                close(fd_in);
            }
            execvp(cmds[i][0], cmds[i]);
            perror("execvp");
            exit(1);
        }

        close(p[1]);
        if (i > 0){
            close(fd_in);
        }
        fd_in = p[0];
        i++;
    }
    if (cmds[0] != NULL){
        close(fd_in);
    }
    while (wait(NULL) > 0);
}

int main(void) {
    char line[1024];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL){
            break;
        }
        char ***cmds = parse(line);
        if (cmds[0] == NULL) { 
            free(cmds); continue; 
        }
        seq_pipe(cmds);

        for (int i = 0; cmds[i] != NULL; i++) free(cmds[i]);
        free(cmds);
    }
    return 0;
}