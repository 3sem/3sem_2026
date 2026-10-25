#ifndef INT_H_
#define INT_H_

#include <math.h>
#include <time.h>
#include <stdlib.h>

typedef double (*func_t)(double);

typedef struct{
    double xmin;
    double xmax;
    double ymax;
    double ymin;
    size_t num;
} points_params_t;

static const points_params_t default_points_param = {
    .xmax = 1,
    .ymax = 1,
    .xmin = 0,
    .ymin = 0,
};

int check_points(points_params_t points, func_t func);

double int_func(double x);
#endif