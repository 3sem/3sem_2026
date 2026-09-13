#include "command.h"
#include "parser.h"
#include "linuxCmd.h"

#include <stdio.h>
#include <assert.h>
#include <malloc.h>

#include <unistd.h>
#include <sys/wait.h>

static void checkProcessStatus( int processStatus, pid_t pid);

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

    // dumpPipeline(commandArgv);

    int pipefd[2];
    pid_t newPid = 0;
    size_t curProcess = 0;
    int fdIn = STDIN_FILENO;

    pid_t* processPids = (pid_t*) calloc( amountProcess, sizeof(pid_t));
    assert(processPids);

    for ( ; curProcess < amountProcess; curProcess++)
    {
        if ( curProcess + 1 <= amountProcess )
        {
            createPipe(pipefd);

            newPid = forkProc();
            if ( newPid < 0 ){
                break;
            }

            if( newPid )
            {
                processPids[curProcess] = newPid;
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
                if( fdIn != STDIN_FILENO ){
                    if ( dupFd2(fdIn, STDIN_FILENO) ) break;
                }

                if( curProcess + 1 < amountProcess ){
                    if( dupFd2(pipefd[1], STDOUT_FILENO) ) break;
                }

                close(pipefd[0]);
                close(pipefd[1]);

                execFromPipe( commandArgv[curProcess][0], commandArgv[curProcess]);
            }
        }
        
    }
    if (fdIn != STDIN_FILENO) {
        close(fdIn);
    }

    int processStatus = 0;
    for( size_t curProcess = 0; curProcess < amountProcess; curProcess++)
    {
        waitpid(processPids[curProcess], &processStatus, 0);
        checkProcessStatus( processStatus, processPids[curProcess]);
        
    }

    free(processPids);
    commandsArrDtor( commandArgv, amountProcess);
}

static void checkProcessStatus( int processStatus, pid_t pid)
{
    if( WIFEXITED(processStatus) ){
        if( !WEXITSTATUS(processStatus) )
        {
            fprintf(stderr, "process %d finished successfully\n", pid);
        }
        else{
            fprintf(stderr, "process %d failed with exit code %d\n", pid, WEXITSTATUS(processStatus));
        }
    }
    else if ( WIFSIGNALED(processStatus) )
    {
        int signalNumber = WTERMSIG(processStatus);

        fprintf(stderr, "process %d killed because received signal %d\n", pid, signalNumber);
    }
    
}

