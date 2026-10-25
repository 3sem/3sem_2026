#include "int.h"
#include <stdio.h>

double int_func(double x){

    return x * x;
}

int check_points(points_params_t points, func_t func){
    double s = 0; 
    double x;
    double y;
    srand((unsigned)time(NULL));

    if (func == NULL) {
        return -1; 
    }


    for (int i = 0; i < points.num; i++) {
        x = (double)rand() / RAND_MAX * (points.xmax - points.xmin) + points.xmin;
        y = (double)rand() / RAND_MAX * (points.ymax - points.ymin) + points.ymin;

        s += (y - func(x)) < 0;
        
    }

    return s;
}