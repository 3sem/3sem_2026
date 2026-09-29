#include "common.h"

int parse_args(char *cmd, char **args) 
{
    int count = 0;
    char *token = strtok(cmd, " \t\n");
    
    while (token && count < MAX_ARGS - 1) 
    {
        while (*token == ' ') token++;
        args[count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[count] = NULL;
    return count;
}

int split_pipeline(char *cmd, char *commands[]) 
{
    int count = 0;
    int in_quotes = 0;
    char *start = cmd;
    
    for (int i = 0; cmd[i] != '\0'; i++) 
    {
        if (cmd[i] == '"' || cmd[i] == '\'') 
            in_quotes = !in_quotes;
        
        if (cmd[i] == '|' && !in_quotes)
        {
            cmd[i] = '\0';  
            commands[count++] = start;
            start = cmd + i + 1;
            
            while (*start == ' ') start++;
        }
    }
    
    if (*start != '\0') 
        commands[count++] = start;
    
    for (int i = 0; i < count; i++)
    {
        while (*commands[i] == ' ') commands[i]++;
        char *end = commands[i] + strlen(commands[i]) - 1;
        while (end > commands[i] && *end == ' ') end--;
        *(end + 1) = '\0';
    }
    return count;
}