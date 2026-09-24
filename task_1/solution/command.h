#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

#define MAX_STR_SIZE 256

enum commandReadCode
{
    READ_FAILURE,
    READ_SUCCESS
};

typedef struct Command
{
    char commandStr[MAX_STR_SIZE];
    enum commandReadCode readStatus;
} Command;


Command readCommand(void);
void    runCommand( Command* command);

#endif /* COMMAND_H */
