#include <stdio.h>
#include <stdlib.h>


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
            if (str[position] == ' ')
            {
                str[position] = '\0';
                array_ptr [i][j] = str + position + 1;
                j++;
            }
        }

        position++;
    }
    

    return array_ptr;
    //printf ("%s\n", array_ptr [0][1]);
    //free (array_ptr[0]);
    //free (array_ptr[1]);
    //free (array_ptr);
}