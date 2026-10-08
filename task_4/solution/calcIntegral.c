#include "calcIntegral.h"

#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include <pthread.h>
#include <unistd.h>

static double randDouble(double min, double max, unsigned int* seed);

const size_t AMOUNT_GEN_POINTS = 100000000;

typedef struct
{
    const expressionTree_t* expression;
    double* result;
    size_t amountPoints;
    double borders[2];
    double sumSquares;
    unsigned int seed;
} generatorArgs_t;

static void* generatorDots(void* args)
{
    generatorArgs_t* params = (generatorArgs_t*) args;
    assert(params);

    for(size_t i = 0; i < params->amountPoints; i++){
        double curX = randDouble(params->borders[0], params->borders[1], &params->seed);

        double value = evaluateExpression(params->expression, curX);
        *params->result += value;
        params->sumSquares += value * value;
    }
    return NULL;
}


double* integrateMonteCarlo(expressionTree_t* expression, double* borders, size_t nThreads){
    assert(expression);
    assert(borders);

    if (nThreads == 0)
        return NULL;

    double x0 = borders[0];
    double x  = borders[1];

    double* integral = calloc(nThreads, sizeof(double));

    pthread_t* threads = calloc(nThreads, sizeof(pthread_t));
    assert(threads);

    generatorArgs_t* args = calloc(nThreads, sizeof(*args));
    assert(args);

    size_t created = 0;
    for(size_t curThread = 0; curThread < nThreads; curThread++){
        args[curThread] = (generatorArgs_t){expression, &integral[curThread],
            AMOUNT_GEN_POINTS / nThreads + (curThread < AMOUNT_GEN_POINTS % nThreads),
            {x0, x}, 0, (unsigned int)curThread + 1};
        int error = pthread_create(&(threads[curThread]), NULL, generatorDots, &args[curThread]);
        if (error != 0)
            break;
        created++;
    }


    for(size_t curThread = 0; curThread < created; curThread++){
        pthread_join(threads[curThread], NULL);
    }
    
    
    double* result = created == nThreads ? calloc(2, sizeof(double)) : NULL;
    if (result == NULL){
        free(integral);
        free(threads);
        free(args);
        return NULL;
    }
    
    for(size_t curThread = 0; curThread < nThreads; curThread++){
        result[0] += integral[curThread];
        result[1] += args[curThread].sumSquares;
    }
    
    double mean = result[0] / AMOUNT_GEN_POINTS;
    double variance = fmax(0.0, (result[1] - result[0] * mean) / (AMOUNT_GEN_POINTS - 1));
    result[0] = (x - x0) * mean;
    result[1] = fabs(x - x0) * sqrt(variance / AMOUNT_GEN_POINTS);

    free(integral);
    free(threads);
    free(args);

    return result;
}

static double randDouble(double min, double max, unsigned int* seed){
    return min + ((double)rand_r(seed) / RAND_MAX) * (max - min);
}
