#include "common.h"

int main() {
    
    while (1) {
        char *cmd = command_reader();
        
        if (cmd == NULL)
            break;
        if (strlen(cmd) == 0) {
            free(cmd);
            continue;
        }
        
        if (strstr(cmd, "|") != NULL) {
            char *commands[MAX_CMDS];
            int cmd_count = split_pipeline(cmd, commands);
            
            if (cmd_count > 0) {
                run_pipeline(commands, cmd_count);
            }
        } else {
            run_one_command(cmd);
        }
        
        free(cmd);
    }
    
    return 0;
}