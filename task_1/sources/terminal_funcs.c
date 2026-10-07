#define _POSIX_C_SOURCE 200809L

#include "terminal_funcs.h"

char *trimWhitespace(char *text) {
	while (*text != '\0' && isspace((unsigned char)*text)) {
		++text;
	}

	if (*text == '\0') {
		return text;
	}
	
	char* end = text + strlen(text) - 1;
	while (end > text && isspace((unsigned char)*end)) {
		*end = '\0';
		--end;
	}

	return text;
}

void freeCommands(Command *commands, size_t command_count) {
    if (commands == NULL) {
        return;
    }

    for (size_t i = 0; i < command_count; ++i) {
        free(commands[i].argv);
    }
    free(commands);
}

int parseArguments(char *segment, char ***argv_out) {
    size_t capacity = 4;
    size_t argument_count = 0;
    char **argv = malloc(capacity * sizeof(*argv));
    if (argv == NULL) {
        perror("malloc");
        return -1;
    }

    char *save_ptr = NULL;
    for (char *token = strtok_r(segment, " \t\r\n", &save_ptr);
         token != NULL;
         token = strtok_r(NULL, " \t\r\n", &save_ptr)) {
        if (argument_count + 1 >= capacity) {
            capacity *= 2;
            char **new_argv = realloc(argv, capacity * sizeof(*new_argv));
            if (new_argv == NULL) {
                perror("realloc");
                free(argv);
                return -1;
            }
            argv = new_argv;
        }

        argv[argument_count++] = token;
    }

    if (argument_count == 0) {
        free(argv);
        return -1;
    }

    argv[argument_count] = NULL;
    *argv_out = argv;
    return 0;
}

int parsePipeline(char *line, Command **commands_out, size_t *command_count_out) {
    size_t capacity = 4;
    size_t command_count = 0;
    Command *commands = calloc(capacity, sizeof(*commands));
    if (commands == NULL) {
        perror("calloc");
        return -1;
    }

    char *segment_start = line;
    for (char *cursor = line;; ++cursor) {
        if (*cursor != '|' && *cursor != '\0') {
            continue;
        }

        char delimiter = *cursor;
        *cursor = '\0';

        char *segment = trimWhitespace(segment_start);
        if (*segment == '\0') {
            fprintf(stderr, "syntax error: empty command in pipeline\n");
            freeCommands(commands, command_count);
            return -1;
        }

        if (command_count == capacity) {
            capacity *= 2;
            Command *new_commands = realloc(commands, capacity * sizeof(*new_commands));
            if (new_commands == NULL) {
                perror("realloc");
                freeCommands(commands, command_count);
                return -1;
            }
            commands = new_commands;
        }

        commands[command_count].argv = NULL;
        if (parseArguments(segment, &commands[command_count].argv) == -1) {
            freeCommands(commands, command_count);
            return -1;
        }
        ++command_count;

        if (delimiter == '\0') {
            break;
        }
        segment_start = cursor + 1;
    }

    *commands_out = commands;
    *command_count_out = command_count;
    return 0;
}

void reportProcessStatus(pid_t pid, const char *command_name, int status) {
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        fprintf(stderr,
                "[%s, pid=%ld] exited with code %d%s\n",
                command_name,
                (long)pid,
                exit_code,
                exit_code == 0 ? " (success)" : "");
        return;
    }

    if (WIFSIGNALED(status)) {
        fprintf(stderr,
                "[%s, pid=%ld] terminated by signal %d\n",
                command_name,
                (long)pid,
                WTERMSIG(status));
        return;
    }

    fprintf(stderr,
            "[%s, pid=%ld] changed state unexpectedly (raw status=%d)\n",
            command_name,
            (long)pid,
            status);
}

int statusToExitCode(int status) {
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return EXIT_FAILURE;
}

int waitForChild(pid_t pid, int *status_out) {
    for (;;) {
        pid_t result = waitpid(pid, status_out, 0);
        if (result == pid) {
            return 0;
        }
        if (result == -1 && errno == EINTR) {
            continue;
        }
        if (result == -1) {
            perror("waitpid");
            return -1;
        }
    }
}

int executePipeline(const Command *commands, size_t command_count) {
    pid_t *pids = calloc(command_count, sizeof(*pids));
    if (pids == NULL) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    size_t launched_count = 0;
    int previous_read_fd = -1;
    bool launch_failed = false;

    for (size_t i = 0; i < command_count; ++i) {
        int next_pipe[2] = {-1, -1};
        bool has_next_command = i + 1 < command_count;

        if (has_next_command && pipe(next_pipe) == -1) {
            perror("pipe");
            launch_failed = true;
            break;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            if (next_pipe[0] != -1) {
                close(next_pipe[0]);
                close(next_pipe[1]);
            }
            launch_failed = true;
            break;
        }

        if (pid == 0) {
            if (previous_read_fd != -1) {
                if (dup2(previous_read_fd, STDIN_FILENO) == -1) {
                    perror("dup2 stdin");
                    _exit(126);
                }
            }

            if (has_next_command) {
                if (dup2(next_pipe[1], STDOUT_FILENO) == -1) {
                    perror("dup2 stdout");
                    _exit(126);
                }
            }

            if (previous_read_fd != -1) {
                close(previous_read_fd);
            }
            if (has_next_command) {
                close(next_pipe[0]);
                close(next_pipe[1]);
            }

            execvp(commands[i].argv[0], commands[i].argv);

            int exec_errno = errno;
            perror(commands[i].argv[0]);
            _exit(exec_errno == ENOENT ? 127 : 126);
        }

        pids[launched_count++] = pid;

        if (previous_read_fd != -1) {
            close(previous_read_fd);
            previous_read_fd = -1;
        }

        if (has_next_command) {
            close(next_pipe[1]);
            previous_read_fd = next_pipe[0];
        }
    }

    if (previous_read_fd != -1) {
        close(previous_read_fd);
    }

    int pipeline_exit_code = launch_failed ? EXIT_FAILURE : 0;

    for (size_t i = 0; i < launched_count; ++i) {
        int status = 0;
        if (waitForChild(pids[i], &status) == -1) {
            pipeline_exit_code = EXIT_FAILURE;
            continue;
        }

        reportProcessStatus(pids[i], commands[i].argv[0], status);

        if (!launch_failed && i + 1 == command_count) {
            pipeline_exit_code = statusToExitCode(status);
        }
    }

    free(pids);
    return pipeline_exit_code;
}
