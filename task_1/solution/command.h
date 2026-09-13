#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

const size_t MAX_STR_SIZE = 256;

enum commandReadCode
{
    READ_FAILURE,
    READ_SUCCESS
};

struct Command
{
    char commandStr[MAX_STR_SIZE];
    commandReadCode readStatus;
};


Command readCommand();
void    runCommand( Command* command);

#endif /* COMMAND_H */