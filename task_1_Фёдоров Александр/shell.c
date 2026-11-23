#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void close_fd(int fd)
{
    if (fd != -1)
        close(fd);
}

static char **parse_args(char *text)
{
    size_t count = 0;
    for (size_t i = 0; text[i]; ++i) {
        if (!isspace((unsigned char)text[i]) &&
            (i == 0 || isspace((unsigned char)text[i - 1])))
            ++count;
    }

    char **args = calloc(count + 1, sizeof(*args));
    if (args == NULL)
        return NULL;
    char *save = NULL;
    size_t i = 0;
    for (char *word = strtok_r(text, " \t\r\n\v\f", &save); word != NULL;
         word = strtok_r(NULL, " \t\r\n\v\f", &save))
        args[i++] = word;
    return args; /* Последний элемент уже равен NULL после calloc. */
}

static int run_pipeline(char ***commands, size_t count)
{
    pid_t *pids = calloc(count, sizeof(*pids));
    if (pids == NULL) {
        perror("calloc");
        return 1;
    }
    size_t started = 0;
    int input = -1, failed = 0, result = 0;

    for (size_t i = 0; i < count; ++i) {
        int fd[2] = {-1, -1};
        if (i + 1 < count && pipe(fd) == -1) {
            perror("pipe");
            failed = 1;
            break;
        }
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            close_fd(fd[0]);
            close_fd(fd[1]);
            failed = 1;
            break;
        }
        if (pid == 0) {
            if ((input != -1 && dup2(input, STDIN_FILENO) == -1) ||
                (fd[1] != -1 && dup2(fd[1], STDOUT_FILENO) == -1)) {
                perror("dup2");
                _exit(126);
            }
            close_fd(input);
            close_fd(fd[0]);
            close_fd(fd[1]);
            execvp(commands[i][0], commands[i]);
            int error = errno;
            perror(commands[i][0]);
            _exit(error == ENOENT || error == ENOTDIR ? 127 : 126);
        }
        pids[started++] = pid;
        close_fd(input);
        close_fd(fd[1]);
        input = fd[0]; /* Родителю нужен только читающий конец канала. */
    }
    close_fd(input);

    /* При ошибке не оставляем части незавершённого конвейера работать. */
    if (failed) {
        for (size_t i = 0; i < started; ++i)
            kill(pids[i], SIGKILL);
    }
    /* Ждём только после запуска всех команд, иначе канал может заполниться. */
    for (size_t i = 0; i < started; ++i) {
        int status;
        pid_t waited;
        do {
            waited = waitpid(pids[i], &status, 0);
        } while (waited == -1 && errno == EINTR);
        if (waited == -1) {
            perror("waitpid");
            failed = 1;
        } else if (WIFEXITED(status)) {
            result = WEXITSTATUS(status);
            fprintf(stderr, "[%zu] %s: exit %d\n", i + 1, commands[i][0], result);
        } else if (WIFSIGNALED(status)) {
            result = 128 + WTERMSIG(status);
            fprintf(stderr, "[%zu] %s: signal %d\n",
                    i + 1, commands[i][0], WTERMSIG(status));
        }
    }
    free(pids);
    return failed ? 1 : result;
}

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    int result = 0;
    /* Данные после строки команды могут понадобиться её дочернему процессу. */
    if (setvbuf(stdin, NULL, _IONBF, 0) != 0)
        return 1;

    while (1) {
        if (isatty(STDIN_FILENO)) {
            fprintf(stderr, "shell> ");
            fflush(stderr);
        }
        if (getline(&line, &capacity, stdin) == -1) {
            if (!feof(stdin)) {
                perror("getline");
                result = 1;
            }
            break;
        }
        if (line[strspn(line, " \t\r\n\v\f")] == '\0')
            continue;

        size_t count = 1;
        for (char *p = line; *p; ++p) {
            if (*p == '|')
                ++count;
        }
        char ***commands = calloc(count, sizeof(*commands));
        if (commands == NULL) {
            perror("calloc");
            result = 1;
            break;
        }

        char *part = line;
        int valid = 1;
        /* Сначала разбираем весь конвейер, чтобы не запускать ошибочную строку. */
        for (size_t i = 0; i < count; ++i) {
            char *next = strchr(part, '|');
            if (next != NULL)
                *next++ = '\0';
            commands[i] = parse_args(part);
            if (commands[i] == NULL) {
                perror("calloc");
                result = 1;
                valid = 0;
                break;
            }
            if (commands[i][0] == NULL) {
                fprintf(stderr, "shell: пустая команда в конвейере\n");
                result = 2;
                valid = 0;
                break;
            }
            part = next;
        }
        if (valid)
            result = run_pipeline(commands, count);
        for (size_t i = 0; i < count; ++i)
            free(commands[i]);
        free(commands);
    }
    free(line);
    return result;
}
