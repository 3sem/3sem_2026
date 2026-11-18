/*Когда завершается потомок, родителю по умолчанию прилетает сигнал SIGCHLD.
Напишем обработчик данного сигнала, который будет обрабатывать статус завершенного потомка в родителе.
Это и будет неблокирующей имплементацией логики, функционально эквивалентной неблокирующему wait.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

static void childHandler(int signum)
{
    (void)signum;
    pid_t pid;
    int status;
    char buf[128];

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        int n;
        if (WIFEXITED(status))
            n = snprintf(buf, sizeof buf, "PID %d exited normally. Exit number: %d\n",
                         pid, WEXITSTATUS(status));
        else if (WIFSTOPPED(status))
            n = snprintf(buf, sizeof buf, "PID %d was stopped by %d\n",
                         pid, WSTOPSIG(status));
        else if (WIFSIGNALED(status))
            n = snprintf(buf, sizeof buf, "PID %d exited due to signal %d\n",
                         pid, WTERMSIG(status));
        else
            continue;
        if (n > 0)
            write(STDOUT_FILENO, buf, (size_t)n);
    }
    if (pid == -1 && errno != ECHILD)
        perror("waitpid");
}

int main(void)
{
    struct sigaction sa = { .sa_handler = childHandler };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    /* ... spawn children ... */
    return EXIT_SUCCESS;
}
