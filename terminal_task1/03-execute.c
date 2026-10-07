#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void execute (char*** array_ptr)
{
    int status = 0;
    int fd[2] = {};
    int fd_in = 0;
    
    char prefix [10] = "./";

    for (int i = 0; array_ptr [i] != NULL; i++)
    {
        fd_in = fd[0];

        if (array_ptr [i + 1] != NULL)
        {
             if (pipe (fd) == -1)
                printf ("pipe error\n");
        }
           
        pid_t pid = fork ();

        if (pid)
        {
            //parent
            waitpid (pid, &status, 0);
            
            if (array_ptr [i + 1] != NULL)
                close (fd[1]);
            
            if (i != 0)
                close (fd_in);
        }
        else
        {
            //child
            
            if (array_ptr[i + 1] != NULL)
                close (fd[0]);

            if (array_ptr[i + 1] != NULL)
                dup2 (fd[1], 1);

            if (i != 0)
                dup2 (fd_in, 0);
            
            //char* arr [2] = {array_ptr[i], NULL};
            //fprintf (stderr, "%s ,%s", array_ptr [i][0], array_ptr[i][1]);
            
            int x = execvp (array_ptr[i][0], array_ptr[i]); 
            fprintf (stderr, "x = %d, exec-error\n", x);
            exit (1); 
        }
    }
}
