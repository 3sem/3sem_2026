#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stddef.h>

int handle_builtin(char ***cmds, size_t num_cmds);

void execute_pipeline(char ***cmd, size_t num_cmds, int verbose_fd);

#endif /* EXECUTOR_H */