#include "include.h"

static const size_t start_cmd_arr_capacity = 16;
static const size_t start_wrd_arr_capacity = 16;


static void add_cmd_arr(cmd_arr_t *cmd_arr, char **wrd_arr);
static void add_cmd    (cmd_arr_t *cmd_arr, char *cmd);
static void add_wrd_arr(wrd_arr_t *wrd_arr, char *wrd);


int split_str(char *string, cmd_arr_t *cmd_arr){
    char *save_ptr1 = NULL;
    for (char *cmd  = strtok_r(string, "|", &save_ptr1);
               cmd != NULL; 
               cmd  = strtok_r(NULL  , "|", &save_ptr1)){

        add_cmd(cmd_arr, cmd);
    }

    return 0;
}

void add_cmd(cmd_arr_t *cmd_arr, char *cmd){
    wrd_arr_t wrd_arr = {
        .size     = 0,
        .capacity = start_wrd_arr_capacity,
        .arr      = calloc(start_wrd_arr_capacity, sizeof(char*)), 
    };
    if (!wrd_arr.arr) perror("Memory fail");

    char *save_ptr2 = NULL;
    for (char *wrd = strtok_r(cmd, split_wrd, &save_ptr2); wrd != NULL; wrd = strtok_r(NULL, split_wrd, &save_ptr2)){
        add_wrd_arr(&wrd_arr, wrd);
    }

    add_cmd_arr(cmd_arr, wrd_arr.arr);

    return;
}


//___________________________wrd_arr____________________________
void add_wrd_arr(wrd_arr_t *wrd_arr, char *wrd){
    wrd_arr->arr[wrd_arr->size] = wrd;
    wrd_arr->size++;

    if (wrd_arr->capacity <= wrd_arr->size){
        wrd_arr->capacity *= 2;
        char **new_wrd_arr = realloc(wrd_arr->arr, wrd_arr->capacity * sizeof(char*));

        if (!new_wrd_arr) perror("Memory fail");

        wrd_arr->arr = new_wrd_arr;
    }

    wrd_arr->arr[wrd_arr->size] = NULL;

    return;
}


//___________________________cmd_arr_________________________________

void add_cmd_arr(cmd_arr_t *cmd_arr, char **wrd_arr){
    cmd_arr->arr[cmd_arr->size] = wrd_arr;
    cmd_arr->size++;

    if (cmd_arr->capacity <= cmd_arr->size){
        cmd_arr->capacity  *= 2;
        char ***new_cmd_arr = realloc(cmd_arr->arr, cmd_arr->capacity * sizeof(char**));

        if (!new_cmd_arr) perror("Memory fail");

        cmd_arr->arr = new_cmd_arr;
    }

    cmd_arr->arr[cmd_arr->size] = NULL;

    return;
}

cmd_arr_t *create_cmd_arr(){
    cmd_arr_t *cmd_arr = (cmd_arr_t *)malloc(sizeof(cmd_arr_t));
    if (!cmd_arr) perror("Memory fail");

    cmd_arr->size      = 0;
    cmd_arr->capacity  = start_cmd_arr_capacity;
    cmd_arr->arr       = (char ***)malloc(sizeof(char**) * start_cmd_arr_capacity);

    if (!cmd_arr->arr) perror("Memory fail");

    return cmd_arr;
}

void destroy_cmd_arr(cmd_arr_t *cmd_arr){
    for (size_t cmd_num = 0; cmd_num < cmd_arr->size; cmd_num++){
        free(cmd_arr->arr[cmd_num]);
    }

    free(cmd_arr->arr);
    free(cmd_arr);

    return;
}