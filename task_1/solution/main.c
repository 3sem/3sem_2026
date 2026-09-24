#include <stdio.h>

#include "command.h"

int main(void)
{
    while ( 1 )
    {
        Command curCommand = readCommand();

        if( curCommand.readStatus == READ_SUCCESS )
        {
            runCommand( &curCommand);
        }
        else
        {
            fprintf( stderr, "parse command failure\n");
            break;
        }
    }


}
