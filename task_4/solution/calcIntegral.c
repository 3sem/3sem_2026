#include "calcIntegral.h"

#include <assert.h>
#include <stdlib.h>
#include <math.h>

static double randDouble(double min, double max);

const size_t AMOUNT_GEN_POINTS = 100000000;

double* integrateMonteCarlo(expressionTree_t* expression, double* borders, int nThreads){
    assert(expression);
    assert(borders);

    double x0 = borders[0];
    double x  = borders[1];

    double integral = 0;

    for(size_t i = 0; i < AMOUNT_GEN_POINTS; i++){
        double curX = randDouble(borders[0], borders[1]);

        integral += evaluateExpression(expression, curX);
    }

    integral *= (x - x0) / AMOUNT_GEN_POINTS; 

    double threshold = 1 / sqrt(AMOUNT_GEN_POINTS);

    double* result = calloc(sizeof(double), 2);
    assert(result);

    result[0] = integral;
    result[1] = threshold;

    return result;
}

static double randDouble(double min, double max){
    return min + ((double)rand() / RAND_MAX) * (max - min);
}
