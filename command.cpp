#include "command.h"
#include "parser.h"
#include "linuxCmd.h"

#include <stdio.h>
#include <assert.h>

#include <unistd.h>
#include <sys/wait.h>

static void checkProcessStatus( int processStatus);

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

        checkProcessStatus( processStatus);

        return;
    }

    size_t amountProcess = 0;
    char*** commandArgv = parseCommand( command->commandStr, &amountProcess);
    assert(commandArgv);

    dumpPipeline(commandArgv);

    int pipefd[2];
    createPipe(pipefd);
    pid_t newPid = 0;
    size_t curProcess = 0;
    for (; curProcess < amountProcess; curProcess++ )
    {
        if( curProcess < amountProcess - 1)
        {
            printf("MEOW\n");
            newPid = fork();

            if( newPid )
            {
                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                }
                close(pipefd[0]);
                close(pipefd[1]);
                break;
            }
            else{
                if (dup2(pipefd[0], STDIN_FILENO) == -1) {
                    perror("dup2");
                }
                close(pipefd[0]);
                close(pipefd[1]);
            }
        }
    }
    
    printf("curProcess = %lu\n", curProcess);
    execvp( commandArgv[curProcess - 1][0], commandArgv[curProcess - 1]);

    perror("execvp failure\n");
}

static void checkProcessStatus( int processStatus)
{
    if( !WEXITSTATUS(processStatus) )
    {
        fprintf(stderr, "process finished successfully\n");
    }
    else{
        fprintf(stderr, "process failed with exit code %d\n", WEXITSTATUS(processStatus));
    }
}

