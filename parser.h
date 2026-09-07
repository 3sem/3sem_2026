#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

char*** parseCommand( char* command, size_t* amountProcess);
void    dumpPipeline( char*** pipeline);

#endif /* PARSER_H */
