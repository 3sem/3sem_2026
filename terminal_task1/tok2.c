#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_parse (char* str, char*** array_ptr);
void arg_parse (char*** array_ptr, int cmd);


char*** tok (char* str)
{
    str [0] = '\0';

    char*** array_ptr = (char***)calloc (8, sizeof (char**));

    int cmd = cmd_parse (str, array_ptr);

    for (int counter = 0; counter < cmd; counter++)
    {
        arg_parse (array_ptr, counter);
    }  
    
    return array_ptr;
}

int cmd_parse (char* str, char*** array_ptr)
{
    int len = strlen (str + 1);
    int cmd = 0;
    for (int counter = 1; str [counter] != '\0'; counter++)
    {
        if (str [counter] == '|')
            str [counter] = '\0';

        if (str [counter] == ' ' && str [counter - 1] == '\0')
            str [counter] = '\0';

        if (str [counter] == ' ' && str [counter + 1] == '|')
        {
            for (int i = 0; str [counter - i] == ' '; i++)
                str [counter - i] = '\0';
        }       
    }

    for (int counter = 0; counter < len; counter++)
    {
        if (str [counter] != '\0' && str [counter - 1] == '\0')
        {
            array_ptr [cmd] = calloc (10, sizeof (char*));
            array_ptr [cmd][0] = str + counter;
            cmd++;
        }
    }

    return cmd;
}

void arg_parse (char*** array_ptr, int cmd)
{
    int arg = 0;
    char* cmd_ptr = array_ptr [cmd][0];
    int cmd_len = strlen (cmd_ptr);

    for (int counter = 0; counter < cmd_len; counter++)
    {
        if (*(cmd_ptr + counter) == ' ' && *(cmd_ptr + counter - 1) != '\\')
            *(cmd_ptr + counter) = '\0';
    }

    for (int counter = 0; counter < cmd_len; counter++)
    {
        if (*(cmd_ptr + counter) == ' ' && *(cmd_ptr + counter - 1) == '\\')
        {
            for (int i = 0; *(cmd_ptr + counter + i - 1) != '\0'; i++)
            {
                *(cmd_ptr + counter + i - 1) = *(cmd_ptr + counter + i);
            }
        }
    }

    for (int counter = 0; counter < cmd_len; counter++)
    {
        if (*(cmd_ptr + counter) != '\0' && *(cmd_ptr + counter - 1) == '\0')
        {
            array_ptr [cmd][arg] = cmd_ptr + counter;
            arg++;
        }
    }


}
    