#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "pipe.h"

int main(void)
{
    while (1) {

        input_t input = {};
        int result = getInput(&input);

        if (result == 0)
            break;

        if (result == -1)
            continue;

        if (input.tokens[0] != NULL && input.tokens[1] == NULL && strcmp(input.tokens[0][0], "exit") == 0) {
            inputCleanup(&input);
            break;
        }

        seqPipe(input.tokens);
        inputCleanup(&input);
    }

    return 0;
}
