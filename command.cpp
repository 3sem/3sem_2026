#include "command.h"
#include "parser.h"

#include <stdio.h>
#include <assert.h>

#include <unistd.h>
#include <sys/wait.h>

Command readCommand()
{
    Command curCommand = { .commandStr = "", .readStatus = READ_SUCCESS};

    if( !fgets(curCommand.commandStr, MAX_STR_SIZE, stdin) )
    {
        curCommand.readStatus = READ_FAILURE;
    }

    return curCommand;
}

void runCommand( Command* command)
{
    assert(command);

    const pid_t pid = fork();

    if ( pid < 0)
    {
        printf("fork failed!\n");
    }

    if( pid )
    {   
        int processStatus = 0;
        waitpid( pid, &processStatus, 0);

        // checkCommandStatus();

        return;
    }

    char** commandArgv = parseCommand( command->commandStr);
    assert(commandArgv);

    execvp( commandArgv[0], commandArgv);

    perror("execvp failure\n");
}


