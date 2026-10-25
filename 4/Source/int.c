#include "int.h"
#include <stdio.h>

double int_func(double x){

    return x * x;
}

int check_points(points_params_t points, func_t func, int seed_add){
    double s = 0; 
    double x;
    double y;
    unsigned int  seed = time(NULL) + seed_add;

    if (func == NULL) {
        return -1; 
    }


    for (int i = 0; i < points.num; i++) {
        x = (double)rand_r(&seed) / RAND_MAX * (points.xmax - points.xmin) + points.xmin;
        y = (double)rand_r(&seed) / RAND_MAX * (points.ymax - points.ymin) + points.ymin;

        s += (y - func(x)) < 0;
        
    }

    return s;
}