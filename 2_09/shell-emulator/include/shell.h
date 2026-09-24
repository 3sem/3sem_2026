#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>

#define BUFF_SIZE 4096
#define ARGS_AM 100
#define COMMANDS_AM 100

typedef struct arg {
    size_t amount;
    char* argv[ARGS_AM];
} arg;


void see_status(int status);
void shell_func();
size_t arr_of_ptr(char* buffer ,arg* commands);
void arr_dump(size_t comm_count ,arg* args_arr);


