#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    char **argv;
    size_t argc;
    size_t capacity;
} Command;

typedef struct {
    Command *commands;
    size_t count;
} Pipeline;

static void free_pipeline(Pipeline *pipeline)
{
    for (size_t index = 0; index < pipeline->count; ++index) {
        Command *command = &pipeline->commands[index];
        for (size_t argument = 0; argument < command->argc; ++argument)
            free(command->argv[argument]);
        free(command->argv);
    }
    free(pipeline->commands);
    *pipeline = (Pipeline){0};
}

static int add_command(Pipeline *pipeline)
{
    Command *commands = realloc(pipeline->commands,
                                (pipeline->count + 1) * sizeof(*commands));
    if (commands == NULL)
        return -1;
    pipeline->commands = commands;
    pipeline->commands[pipeline->count++] = (Command){0};
    return 0;
}

static int add_argument(Command *command, const char *word)
{
    if (command->argc + 1 >= command->capacity) {
        size_t capacity = command->capacity ? command->capacity * 2 : 8;
        char **arguments = realloc(command->argv, capacity * sizeof(*arguments));
        if (arguments == NULL)
            return -1;
        command->argv = arguments;
        command->capacity = capacity;
    }
    char *argument = strdup(word);
    if (argument == NULL)
        return -1;
    command->argv[command->argc++] = argument;
    command->argv[command->argc] = NULL;
    return 0;
}

static int parse_pipeline(const char *line, Pipeline *pipeline)
{
    char *word = malloc(strlen(line) + 1);
    if (word == NULL)
        return -1;
    if (add_command(pipeline) < 0) {
        free(word);
        return -1;
    }

    size_t length = 0;
    int started = 0;
    char quote = 0;
    for (size_t index = 0;; ++index) {
        char current = line[index];
        if (current == '\0' && quote != 0)
            goto syntax_error;

        if (current == '\\' && quote != '\'') {
            char next = line[++index];
            if (next == '\0' || next == '\n')
                goto syntax_error;
            if (quote == '"' && next != '"' && next != '\\' &&
                next != '$' && next != '`')
                word[length++] = '\\';
            word[length++] = next;
            started = 1;
        } else if (quote != 0) {
            if (current == quote)
                quote = 0;
            else
                word[length++] = current;
        } else if (current == '\'' || current == '"') {
            quote = current;
            started = 1;
        } else if (current == '\0' || current == '|' ||
                   isspace((unsigned char)current)) {
            Command *command = &pipeline->commands[pipeline->count - 1];
            if (started) {
                word[length] = '\0';
                if (add_argument(command, word) < 0)
                    goto error;
                length = 0;
                started = 0;
            }
            if (current == '|') {
                if (command->argc == 0)
                    goto syntax_error;
                if (add_command(pipeline) < 0)
                    goto error;
            }
            if (current == '\0') {
                if (pipeline->count > 1 && command->argc == 0)
                    goto syntax_error;
                break;
            }
        } else {
            word[length++] = current;
            started = 1;
        }
    }
    free(word);
    return 0;

syntax_error:
    errno = EINVAL;
error:
    free(word);
    free_pipeline(pipeline);
    return -1;
}

static int run_pipeline(const Pipeline *pipeline)
{
    pid_t *children = calloc(pipeline->count, sizeof(*children));
    if (children == NULL) {
        perror("calloc");
        return 1;
    }

    size_t launched = 0;
    int previous_read = -1;
    int failed = 0;
    for (size_t index = 0; index < pipeline->count; ++index) {
        int next_pipe[2] = {-1, -1};
        if (index + 1 < pipeline->count && pipe(next_pipe) < 0) {
            perror("pipe");
            failed = 1;
            break;
        }

        pid_t child = fork();
        if (child < 0) {
            perror("fork");
            if (next_pipe[0] >= 0) {
                close(next_pipe[0]);
                close(next_pipe[1]);
            }
            failed = 1;
            break;
        }
        if (child == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            if ((previous_read >= 0 && dup2(previous_read, STDIN_FILENO) < 0) ||
                (next_pipe[1] >= 0 && dup2(next_pipe[1], STDOUT_FILENO) < 0)) {
                perror("dup2");
                _exit(126);
            }
            if (previous_read >= 0)
                close(previous_read);
            if (next_pipe[0] >= 0) {
                close(next_pipe[0]);
                close(next_pipe[1]);
            }
            execvp(pipeline->commands[index].argv[0], pipeline->commands[index].argv);
            int saved_errno = errno;
            perror(pipeline->commands[index].argv[0]);
            _exit(saved_errno == ENOENT ? 127 : 126);
        }

        children[launched++] = child;
        if (previous_read >= 0)
            close(previous_read);
        if (next_pipe[1] >= 0)
            close(next_pipe[1]);
        previous_read = next_pipe[0];
    }
    if (previous_read >= 0)
        close(previous_read);
    if (failed) {
        for (size_t index = 0; index < launched; ++index)
            kill(children[index], SIGTERM);
    }

    int result = failed ? 1 : 0;
    for (size_t index = 0; index < launched; ++index) {
        int status;
        pid_t waited;
        do {
            waited = waitpid(children[index], &status, 0);
        } while (waited < 0 && errno == EINTR);
        if (waited < 0) {
            perror("waitpid");
            result = 1;
            continue;
        }
        if (WIFEXITED(status)) {
            fprintf(stderr, "[%ld] %s: exit %d\n", (long)children[index],
                    pipeline->commands[index].argv[0], WEXITSTATUS(status));
            if (!failed && index + 1 == launched)
                result = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "[%ld] %s: signal %d\n", (long)children[index],
                    pipeline->commands[index].argv[0], WTERMSIG(status));
            if (!failed && index + 1 == launched)
                result = 128 + WTERMSIG(status);
        }
    }
    free(children);
    return result;
}

int main(void)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    setvbuf(stdin, NULL, _IONBF, 0);

    char *line = NULL;
    size_t capacity = 0;
    int last_status = 0;
    while (1) {
        if (isatty(STDIN_FILENO)) {
            fputs("shell> ", stderr);
            fflush(stderr);
        }
        if (getline(&line, &capacity, stdin) < 0) {
            if (ferror(stdin)) {
                perror("getline");
                last_status = 1;
            }
            break;
        }

        Pipeline pipeline = {0};
        if (parse_pipeline(line, &pipeline) < 0) {
            if (errno == EINVAL)
                fputs("Invalid command or unclosed quote\n", stderr);
            else
                perror("parse");
            last_status = 2;
            continue;
        }
        if (pipeline.commands[0].argc == 0) {
            free_pipeline(&pipeline);
            continue;
        }
        if (pipeline.count == 1 &&
            strcmp(pipeline.commands[0].argv[0], "exit") == 0) {
            free_pipeline(&pipeline);
            break;
        }
        last_status = run_pipeline(&pipeline);
        free_pipeline(&pipeline);
    }
    free(line);
    return last_status;
}
