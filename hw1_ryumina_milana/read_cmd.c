#include <stdio.h>
#include <assert.h>

#include "common.h"
#include "command_parser.h"

#define CMD_BUFFER 1024

char* ReadCmd(void)
{
    char *string_cmd = (char*)calloc(CMD_BUFFER, sizeof(char));
    if (string_cmd == NULL) {
        return NULL;
    }

    printf(COLOR_MAGENTA "Milana-shell> " COLOR_RESET);
    fflush(stdout);

    if (fgets(string_cmd, CMD_BUFFER, stdin) == NULL) {
        free(string_cmd);      /* <-- освобождаем перед возвратом NULL */
        return NULL;
    }

    return string_cmd;
}
