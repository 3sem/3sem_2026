#include <stdio.h>

#include "tok.h"
#include "execute.h"

int release (char*** array_ptr);

//./a, ./b, ./c, ./d, ./e - прогоночные программы

int main ()
{
    char str [200] = {};

    fgets (str + 1, 190, stdin);
    str [strlen (str + 1)] = '\0';

    char*** array_ptr = tok (str);

    execute (array_ptr);

    release (array_ptr);
}

int release (char *** array_ptr)
{
    for (int i = 0; array_ptr [i] != NULL; i++)
    {
        free (array_ptr [i]);
    }
    free (array_ptr);
}