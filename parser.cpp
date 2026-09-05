#include "parser.h"
#include "general.h"

#include <assert.h>
#include <string.h>
#include <malloc.h>

const char* STRTOK_DELIM          = " \t\r\n";
const size_t START_AMOUNT_OF_ARGS = 2;

char** parseCommand( char* command)
{
    assert(command);

    size_t capacity = START_AMOUNT_OF_ARGS;
    size_t argsCount = 0;
    char** parsedArgs = (char**) calloc(capacity, sizeof(char*));
    assert(parsedArgs);

    char* savePtr = nullptr;
    char* token = strtok_r(command, STRTOK_DELIM, &savePtr);

    while (token)
    {
        if (argsCount + 1 >= capacity)
        {
            capacity *= 2;
            parsedArgs = reallocArr(parsedArgs, capacity * sizeof(*parsedArgs));
        }

        parsedArgs[argsCount++] = token;
        token = strtok_r(nullptr, STRTOK_DELIM, &savePtr);
    }

    parsedArgs[argsCount] = nullptr;
    return parsedArgs;
}

