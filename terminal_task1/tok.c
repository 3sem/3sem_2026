#include <stdio.h>
#include <stdlib.h>

void remove_symbol (char* str);
void my_remove (char*** array_ptr);


char*** tok (char* str)
{
    str [0] = 'a';

    char*** array_ptr = (char***)calloc (8, sizeof (char**));
    int position = 1;

    
    for (int i = 0; str[position - 1] != '\0' ; i++)
    {
        str[position - 1] = '\0';
        

        array_ptr [i] = (char**)calloc (8, sizeof (char*));
        
        array_ptr [i][0] = str + position;

        for (int j = 1; str[position] != '|' && str[position] != '\0'; position++)
        {
            if (str[position] == ' ' && str[position - 1] != '\\')
            {
                str[position] = '\0';
                array_ptr [i][j] = str + position + 1;
                j++;
            }
        }

        position++;
    }

    my_remove (array_ptr);

    return array_ptr;
    //printf ("%s\n", array_ptr [0][1]);
    //free (array_ptr[0]);
    //free (array_ptr[1]);
    //free (array_ptr);
}

void my_remove (char*** array_ptr)
{
    for (int i = 0; array_ptr [i] != NULL; i++)
    {
        for (int j = 0; array_ptr [i][j] != NULL; j++)
        {
            remove_symbol (array_ptr [i][j]);
        }
    }
}

void remove_symbol (char* str)
{
    for (int i = 0; str [i] != '\0'; i++)
    {
        if (str [i] == '\\')
        {
            for (int j = i; str [j] != '\0'; j++)
            {
                str [j] = str [j + 1];
            }
        }
    }
}