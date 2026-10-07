#ifndef CALC_INTEGRAL_H
#define CALC_INTEGRAL_H

#include "expression.h"

double* integrateMonteCarlo(expressionTree_t* expression, double* borders, int nThreads);

#endif /* CALC_INTEGRAL_H */
