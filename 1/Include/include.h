#ifndef INCLUDE_H_
#define INCLUDE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/wait.h>

static const char split_cmd[] = "|";
static const char split_wrd[] = " ";

typedef struct cmd_arr_struct{
    size_t  size;
    size_t  capacity;
    char*** arr;
} cmd_arr_t;

typedef struct wrd_arr_struct{
    size_t  size;
    size_t  capacity;
    char**  arr;
} wrd_arr_t;

cmd_arr_t *create_cmd_arr ();
void       destroy_cmd_arr(cmd_arr_t *cmd_arr);

int  split_str(char *string, cmd_arr_t *cmd_arr_t);
void seq_pipeline(char ***cmd);

#endif