#include "parser.h"

#include "calcIntegral.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

int main(int argc, char** argv)
{
    char error[128];
    expressionTree_t function;
    expressionTreeCtor(&function);

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s EXPRESSION N_THREADS\n", argv[0]);
        return 1;
    }

    if (parseFunction(&function, argv[1], error, sizeof(error)) != 0)
    {
        fprintf(stderr, "Parse error: %s\n", error);
        return 1;
    }

    unsigned long long count = strtoull(argv[2], NULL, 10);

    double borders[2] = {0, 2};
    double* result    = integrateMonteCarlo(&function, borders, (size_t)count);
    if (result == NULL){
        fprintf(stderr, "Failed to allocate memory or create threads\n");
        expressionTreeDtor(&function);
        return 1;
    }
    printf("integral of %s in [%lf, %lf] = %lf +- %lf\n", argv[1], borders[0], borders[1], result[0], result[1]);

    free(result);

    expressionTreeDtor(&function);
    return 0;
}
