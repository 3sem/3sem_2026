#include "shell.h"

int main(void) {
    shell_func();
    return 0;
}

        //if the input in shell is ls| grep a.|wc, 
        //then the result is forming an array commands,
        //that looks: command[0][1]: ls
        //            command[0][2]: null
        //            command[1][0]: grep
        //            command[1][1]: a.
        //            command[1][2]:null and so on
        //the returning value is the amount of blocks of commands
size_t arr_of_ptr(char* buffer, arg* commands) {
    assert(commands);
    assert(buffer);

    size_t counter = 0;
    if(fgets(buffer, BUFF_SIZE , stdin)) {
        while(*buffer != '\n') {
            
            while(*buffer == ' ') { 
                *buffer = '\0';
                buffer++;
            }
            if (*buffer == '|') {
                *buffer = '\0';
                buffer++;

                commands[counter].argv[commands[counter].amount] = NULL;
                counter++;
                continue;
            }

                        
            if(*buffer!='\n') {
                commands[counter].argv[commands[counter].amount] = buffer;
                commands[counter].amount++;
            }
            else {
                *buffer = '\0';
                break;
            }
            while(*buffer != ' ' && *buffer != '|' && *buffer != '\n')  {
                buffer++;
            }

            if (*buffer == '\n') {
                *buffer = '\0';
                break;
            }

        }
        commands[counter].argv[commands[counter].amount] = NULL;
    }
    else {
        fprintf(stderr, "input err\n");
    }
    return counter+1;
}
//this is the emulator of shell,
//that uses pipes and forks to make possible
//constructions like: env| grep HOSTNAME | wc
void shell_func() {
    while(1) {
        printf("> ");

        arg args_arr[COMMANDS_AM] = {};
        char buffer[BUFF_SIZE];

        size_t comm_count = arr_of_ptr(buffer, args_arr);
        if (comm_count == 1 && args_arr[0].amount == 0) {       //if the input is empty
            comm_count--;
        } 
        
        int prev_fd = -1;
        pid_t pids[COMMANDS_AM];

        for (size_t i = 0; i < comm_count; i++) {
            
            int next_fd[2];

            if (i < comm_count - 1) {
                if (pipe(next_fd) == -1) {
                     perror("pipe");
                     abort();
                }
            }

            pid_t pid = fork();

            if (pid == -1) {    
                perror("fork");
                abort();
            }

            if (pid == 0) { 
                if (prev_fd != -1) {
                    dup2(prev_fd, STDIN_FILENO);
                    close(prev_fd);
                }

                
                if (i < comm_count - 1) {
                    dup2(next_fd[1] , STDOUT_FILENO);
                    close(next_fd[0]);
                    close(next_fd[1]);
                }
                execvp(args_arr[i].argv[0], args_arr[i].argv);
                perror("execvp");
                exit(1);

            }

            pids[i] = pid;

            if(prev_fd != -1) {
                close(prev_fd);
            }

            if( i < comm_count - 1) {
                close(next_fd[1]);

                prev_fd = next_fd[0];
            }

        }
        if(prev_fd != -1) {
           close(prev_fd);
        }

        for(size_t i = 0; i < comm_count; i++) {
            int status;
            waitpid(pids[i], &status, 0);
            see_status(status);
        }
    }
}

void arr_dump(size_t comm_count ,arg* args_arr) {
    for (size_t i = 0; i < comm_count; i++) {
        for (size_t j = 0 ; j <= args_arr[i].amount; j++) {
            printf("command[%ld][%ld]%s$\n",i , j, args_arr[i].argv[j]);
        }
    }
}

void see_status(int status) {
    if (WIFEXITED(status)) {
        printf("Child exit code = %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status)) {
        printf("Process was ended by signal - %d\n", WTERMSIG(status));
    }
}
