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

    size_t amountProcess = 0;
    char*** commandArgv = parseCommand( command->commandStr, &amountProcess);
    assert(commandArgv);

    dumpPipeline(commandArgv);

    int pipefd[2];
    pid_t newPid = 0;
    pid_t oldPid = 0;
    size_t curProcess = 0;
    int fdIn = STDIN_FILENO;

    fprintf(stderr, "amountProcess = %lu\n", amountProcess);
    for (; curProcess < amountProcess; curProcess++ )
    {
        if((curProcess == 0) && (curProcess + 1 != amountProcess))
        {
            createPipe(pipefd);

            newPid = fork();
            
            if( newPid )
            {   
                close(pipefd[0]);
                close(pipefd[1]);
                continue;
            }
            else if ( newPid == 0 )
            {
                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                }
                close(pipefd[0]);
                close(pipefd[1]);

                execvp( commandArgv[curProcess][0], commandArgv[curProcess]);
                perror("execvp failure\n");
            }
            
        }
        else if ( (curProcess == 0) && (curProcess + 1 == amountProcess) )
        {
            newPid = fork();

            if( newPid )
            {
                continue;
            }
            else if ( newPid == 0 )
            {
                execvp( commandArgv[curProcess][0], commandArgv[curProcess]);
                perror("execvp failure\n");
            }
            
        }
        else if( (curProcess != 0) && (curProcess + 1 == amountProcess) )
        {
            createPipe(pipefd);

            newPid = fork();

            if( newPid )
            {
                close(pipefd[0]);
                close(pipefd[1]);
                continue;
            }
            else if ( newPid == 0 )
            {
                if (dup2(pipefd[0], STDIN_FILENO) == -1) {
                    perror("dup2");
                }
                close(pipefd[0]);
                close(pipefd[1]);

                execvp( commandArgv[curProcess][0], commandArgv[curProcess]);
                perror("execvp failure\n");
            }
        }
        else if ( curProcess + 1 < amountProcess)
        {
            createPipe(pipefd);

            newPid = fork();

            if( newPid )
            {
                close(pipefd[1]);
                if( curProcess > 0 )
                {
                    close(fdIn);
                }
                fdIn = pipefd[0];
                continue;
            }
            else if ( newPid == 0 )
            {



                if (dup2(pipefd[0], STDIN_FILENO) == -1) {
                    perror("dup2");
                }

                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                }
                close(pipefd[0]);
                close(pipefd[1]);

                execvp( commandArgv[curProcess][0], commandArgv[curProcess]);
                perror("execvp failure\n");
            }
        }
        
    }
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

