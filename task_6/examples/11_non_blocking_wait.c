/*Когда завершается потомок, родителю по умолчанию прилетает сигнал SIGCHLD.
Напишем обработчик данного сигнала, который будет обрабатывать статус завершенного потомка в родителе.
Это и будет неблокирующей имплементацией логики, функционально эквивалентной неблокирующему wait.

signal(SIGCHLD, childHandler);

void childHandler(int signum)
{

    pid_t childpid;
    int childstatus;

    while ((childpid = waitpid( -1, &childstatus, WNOHANG)) > 0)
    {
        if (WIFEXITED(childStatus))
        {
            printf("PID %d exited normally.  Exit number:  %d\n", childpid, WEXITSTATUS(childStatus));
        }
        else
            if (WIFSTOPPED(childStatus))
            {
                printf("PID %d was stopped by %d\n", childpid, WSTOPSIG(childStatus));
            }
            else
                if (WIFSIGNALED(childStatus))
                {
                    printf("PID %d exited due to signal %d\n.", childpid, WTERMSIG(childStatus));
                }
                else
                {
                    perror("waitpid");
                }
        }
    }
}
