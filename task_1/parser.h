#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

char **parse_single_command(const char *cmd_str);

char ***parse_pipeline(const char *line, size_t *out_num_cmds);

void free_argv(char **argv);

void free_pipeline(char ***cmds);

#endif /* PARSER_H */