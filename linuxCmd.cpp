#include "linuxCmd.h"

#include <unistd.h>
#include <assert.h>
#include <stdio.h>

const int COMMAND_NOT_FOUND_UNIX_CODE = 127;

int createPipe( int* fd)
{
    assert( fd);

    if( pipe(fd) == -1 )
    {
        perror( "pipe open failed\n");
        return 1;
    }

    return 0;
}

int dupFd2( int dst, int src)
{
    if ( dup2(dst, src) == -1 ) {
        perror( "dup2 failure");

        return 1;
    }
    return 0;
}

int execFromPipe( const char* fileName, char* const* args)
{
    execvp( fileName, args);
    perror( "no such command\n");
    _exit( COMMAND_NOT_FOUND_UNIX_CODE);
}

pid_t forkProc()
{
    pid_t pid = fork();
    if (pid < 0) {
       printf("Fork failed!\n");
    }
    return pid;   
}