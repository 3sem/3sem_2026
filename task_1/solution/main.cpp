#include <stdio.h>

#include "command.h"

int main()
{
    while ( true )
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

