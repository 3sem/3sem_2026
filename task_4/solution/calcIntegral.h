#ifndef CALC_INTEGRAL_H
#define CALC_INTEGRAL_H

#include <stddef.h>

#include "expression.h"

double* integrateMonteCarlo(expressionTree_t* expression, double* borders, size_t nThreads);

#endif /* CALC_INTEGRAL_H */
