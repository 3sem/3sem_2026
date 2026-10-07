#include "common.h"
#include "run_cmd.h"

#include <unistd.h>
#include <sys/wait.h>

void RunCmd(CommandLine *cline)
{
    assert(cline);

    if (cline->cmd_count == 0) {
        return;
    }

    pid_t pids[MAX_COMMANDS];   
    int   prev_pipe_read = -1;

    for (size_t i = 0; i < cline->cmd_count; i++) {
        int pipeFd[2] = {-1, -1};

        /* Создаём pipe ТОЛЬКО если после текущей команды есть ещё одна. экономим дескрипторы.*/
        if (i < cline->cmd_count - 1) {
            if (pipe(pipeFd) < 0) {
                perror("pipe");
                /* Закрываем всё, что успели открыть в родителе */
                if (prev_pipe_read != -1) {
                    close(prev_pipe_read);
                }
                return;
            }
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            if (prev_pipe_read != -1) {
                close(prev_pipe_read);
            }
            if (i < cline->cmd_count - 1) {
                close(pipeFd[0]);
                close(pipeFd[1]);
            }
            return;
        }
        else if (pid == 0) {
            /* ---------- Дочерний процесс*/

            /* stdin <- чтение из предыдущего pipe */
            if (prev_pipe_read != -1) {
                if (dup2(prev_pipe_read, STDIN_FILENO) < 0) {
                    perror("dup2 stdin");
                    exit(1);
                }
                close(prev_pipe_read);
            }

            /* stdout -> запись в текущий pipe (если есть следующая команда) */
            if (i < cline->cmd_count - 1) {
                if (dup2(pipeFd[1], STDOUT_FILENO) < 0) {
                    perror("dup2 stdout");
                    exit(1);
                }
                close(pipeFd[0]);
                close(pipeFd[1]);
            }

            execvp(cline->cmds[i].argv[0], cline->cmds[i].argv);

            /* execvp вернулся -> ошибка */
            perror("execvp");
            exit(127);
        }
        else {
            /* ---------- Родительский процесс */

            pids[i] = pid;

            /* Родителю больше не нужен read-end предыдущего pipe */
            if (prev_pipe_read != -1) {
                close(prev_pipe_read);
            }

            /* Закрываем write-end текущего pipe, read-end сохраняем для следующего ребёнка */
            if (i < cline->cmd_count - 1) {
                close(pipeFd[1]);
                prev_pipe_read = pipeFd[0];
            }
        }
    }

    /* Ожидаем КАЖДОГО ребёнка по его конкретному pid */
    for (size_t i = 0; i < cline->cmd_count; i++) {
        int status = 0;

        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
            continue;
        }

        if (WIFEXITED(status)) {
            printf("Command %zu ('%s') exited with code %d\n",
                   i, cline->cmds[i].argv[0], WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)) {
            printf("Command %zu ('%s') killed by signal %d\n",
                   i, cline->cmds[i].argv[0], WTERMSIG(status));
        }
        else {
            printf("Command %zu ('%s') terminated abnormally (status=0x%x)\n",
                   i, cline->cmds[i].argv[0], status);
        }
    }
}