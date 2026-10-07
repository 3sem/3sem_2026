#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    char error[128];
    expressionTree_t function;
    expressionTreeCtor(&function);

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s EXPRESSION X\n", argv[0]);
        return 1;
    }

    if (parseFunction(&function, argv[1], error, sizeof(error)) != 0)
    {
        fprintf(stderr, "Parse error: %s\n", error);
        return 1;
    }

    printf("%.17g\n", evaluateExpression(&function, strtod(argv[2], NULL)));
    expressionTreeDtor(&function);
    return 0;
}
