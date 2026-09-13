#include "parser.h"
#include "general.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

const char* STRTOK_DELIM          = " \t\r\n";
const size_t START_AMOUNT_OF_ARGS = 2;
const size_t MAX_PIPELINE_LENGTH  = 64;

char*** parseCommand( char* command, size_t* amountProcess)
{
    assert(command);
    assert(amountProcess);

    char*** pipeLine = (char***) calloc(MAX_PIPELINE_LENGTH, sizeof(char**));
    assert(pipeLine);

    char* savePtr = nullptr;
    char* token = strtok_r(command, STRTOK_DELIM, &savePtr);

    *amountProcess = 0;
    while( token )
    {
        size_t capacity = START_AMOUNT_OF_ARGS;
        size_t argsCount = 0;
        char** parsedArgs = (char**) calloc(capacity, sizeof(char*));
        assert(parsedArgs);

        while ( token )
        {
            if( *token == '|' )
            {
                break;
            }

            if (argsCount + 1 >= capacity)
            {
                capacity *= 2;
                parsedArgs = reallocArr(parsedArgs, capacity * sizeof(*parsedArgs));
            }

            parsedArgs[argsCount++] = token;
            token = strtok_r(nullptr, STRTOK_DELIM, &savePtr);
        }

        parsedArgs[argsCount] = nullptr;

        if( *amountProcess >= MAX_PIPELINE_LENGTH - 1)
        {
            fprintf( stderr, "pipeline maximum length is %lu\n", MAX_PIPELINE_LENGTH);
            break;
        }

        pipeLine[(*amountProcess)++] = parsedArgs;
        token = strtok_r(nullptr, STRTOK_DELIM, &savePtr);
    }

    return pipeLine;
}

void dumpPipeline( char*** pipeline)
{
    assert( pipeline);

    fprintf( stderr, "pipeline:\n");

    for (size_t processIndex = 0;
         pipeline[processIndex] != nullptr;
         processIndex++)
    {
        fprintf( stderr, "  process[%zu]:\n", processIndex);

        for ( size_t argIndex = 0;
             pipeline[processIndex][argIndex] != nullptr;
             argIndex++)
        {
            fprintf( stderr, "    argv[%zu] = \"%s\"\n",
                    argIndex, pipeline[processIndex][argIndex]);
        }
    }
}

void commandsArrDtor( char*** arr, size_t arrSize){
    for( size_t i = 0; i < arrSize; i++)
    {
        free(arr[i]);
        arr[i] = NULL;
    }
    free(arr);
    arr = NULL;
}
