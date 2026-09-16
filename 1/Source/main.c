
#include "include.h"

int main() {
    char command[1024];

    while (1){
        cmd_arr_t *cmd_arr = create_cmd_arr();

        if (scanf(" %[^\n]", command) == -1){
            printf("exit\n");

            destroy_cmd_arr(cmd_arr);
            break;
        }
        if (strcmp(command, "q") == 0){
            
            printf("exit\n");

            destroy_cmd_arr(cmd_arr);
            break;
        }

        split_str(command, cmd_arr);

        seq_pipeline(cmd_arr->arr);

        destroy_cmd_arr(cmd_arr);
    }

    return 0;
}