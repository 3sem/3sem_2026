#ifndef PARSER_H
#define PARSER_H

char*** parseCommand(  char* command);
void    dumpPipeline( char*** pipeline, size_t* amountProcess);

#endif /* PARSER_H */
