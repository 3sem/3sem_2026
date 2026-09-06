#include "linuxCmd.h"

#include <unistd.h>
#include <assert.h>

int createPipe( int* fd)
{
    assert(fd);

    if(pipe(fd) == -1 )
    {
        perror("pipe open failed\n");
        return 1;
    }

    return 0;
}