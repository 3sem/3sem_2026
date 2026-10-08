#include "parser.h"
#include "calcIntegral.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv)
{
    if(argc != 2) return 1;
    size_t nThreads = (size_t)strtoull(argv[1], NULL, 10);
    const char* expression = "x*x";
    double borders[2] = {0, 2};

    expressionTree_t function;
    expressionTreeCtor(&function);
    char error[128];
    if(parseFunction(&function, expression, error, sizeof(error)) != 0){
        fprintf(stderr, "Parse error: %s\n", error);
        return 1;
    }

    struct timespec start;
    struct timespec finish;
    if(clock_gettime(CLOCK_MONOTONIC, &start) != 0){
        expressionTreeDtor(&function);
        return 1;
    }
    double* result = integrateMonteCarlo(&function, borders, nThreads);
    if(clock_gettime(CLOCK_MONOTONIC, &finish) != 0){
        free(result);
        expressionTreeDtor(&function);
        return 1;
    }
    if(result == NULL){
        fprintf(stderr, "Calculation failed for %zu threads\n", nThreads);
        expressionTreeDtor(&function);
        return 1;
    }

    double seconds = (double)(finish.tv_sec - start.tv_sec) +
                     (double)(finish.tv_nsec - start.tv_nsec) / 1e9;
    printf("%.9f,%.17g,%.17g\n", seconds, result[0], result[1]);

    free(result);
    expressionTreeDtor(&function);
    return 0;
}
