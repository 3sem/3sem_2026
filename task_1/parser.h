#ifndef PARSER_H
#define PARSER_H

typedef struct {
    char **argv;
} Command;

typedef struct {
    Command *commands;
    int count;
    char *line;
    char **args;
} Pipeline;

int parse_line(const char *line, Pipeline *p);
void free_line(Pipeline *p);

#endif
