#include "common.h"

char *command_reader() 
{
    char *string = NULL;
    size_t size = 0;
    
    printf("\033[38;5;205m" "my-cmd-emulator> " "\033[0m\n");
    
    if (getline(&string, &size, stdin) == -1) 
    {
        fprintf(stderr, "Error reading string\n");
        free(string);
        return NULL;
    }
    
    string[strcspn(string, "\n")] = '\0';
    
    return string;  
}