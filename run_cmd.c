#include "common.h"

int run_one_command(char *cmd) {
    char *args[MAX_ARGS];
    int arg_count = parse_args(cmd, args);
    
    if (arg_count == 0) return 0;
    
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }
    
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            chdir(getenv("HOME"));
        } else {
            if (chdir(args[1]) != 0) {
                fprintf(stderr, "cd: %s: Нет такого файла или каталога\n", args[1]);
            }
        }
        return 0;
    }
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {
        execvp(args[0], args);
        
        if (errno == ENOENT) {
            fprintf(stderr, "Команда не найдена: %s\n", args[0]);
        } else if (errno == EACCES) {
            fprintf(stderr, "Нет прав на выполнение: %s\n", args[0]);
        } else 
            perror("execvp");
        exit(127);
    } 
    else {
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            if (code != 0) {
                printf("Код возврата: %d\n", code);
            } else {
                printf("Код возврата: %d\n", code);
            }
            return code;
        } else if (WIFSIGNALED(status)) {
            printf("Процесс завершен сигналом: %d\n", WTERMSIG(status));
            return -WTERMSIG(status);
        }
    }
    
    return -1;
}

int run_pipeline(char *commands[], int cmd_count) {
    int num_pipes = cmd_count - 1;
    int (*pipes)[2] = NULL;
    if (num_pipes > 0) {
        pipes = malloc(num_pipes * sizeof(int[2]));
        if (!pipes) {
            perror("malloc");
            return -1;
        }
        
        for (int i = 0; i < num_pipes; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                free(pipes);
                return -1;
            }
        }
    }
    
    for (int i = 0; i < cmd_count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            for (int j = 0; j < num_pipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            free(pipes);
            return -1;
        }
        
        if (pid == 0) {            
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            if (i < cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            for (int j = 0; j < num_pipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            char *args[MAX_ARGS];
            parse_args(commands[i], args);
            
            execvp(args[0], args);
            
            fprintf(stderr, "Ошибка: команда '%s' не найдена\n", args[0]);
            exit(127);
        }
    }
    for (int i = 0; i < num_pipes; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    free(pipes);
    int last_exit_code = 0;
    for (int i = 0; i < cmd_count; i++) {
        int status;
        pid_t pid = wait(&status);
        
        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            if (i == cmd_count - 1) {
                last_exit_code = code;
            }
            printf("Процесс %d (команда %d) завершился с кодом: %d\n", 
                   pid, i + 1, code);
        } else if (WIFSIGNALED(status)) {
            printf("Процесс %d (команда %d) завершен сигналом: %d\n", 
                   pid, i + 1, WTERMSIG(status));
        }
    }
    
    return last_exit_code;
}