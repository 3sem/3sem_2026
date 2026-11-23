#define _GNU_SOURCE

#include "calculator.h"

#include <stdlib.h>
#include <time.h>

double Func(double x) {
    return 1.0 / (x + 1.0);
}

double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

void *Calculate(void *arg) {
    Calculator *calculator = (Calculator *)arg;

    size_t hits = 0;
    unsigned int seed = calculator->seed;
    const double minX = calculator->minX;
    const double spanX = calculator->maxX - minX;
    const double maxY = calculator->maxY;
    const MathFunc fn = calculator->func;
    const size_t dots = calculator->dots_count;

    for (size_t i = 0; i < dots; i++) {
        double r_x = minX + ((double)rand_r(&seed) / (double)RAND_MAX) * spanX;
        double r_y = ((double)rand_r(&seed) / (double)RAND_MAX) * maxY;

        if (r_y < fn(r_x)) {
            if (calculator->mutex && calculator->shared_sum) {
                /* Режим с разделяемой ячейкой и синхронизацией на каждом попадании */
                pthread_mutex_lock(calculator->mutex);
                *(calculator->shared_sum) += 1.0;
                pthread_mutex_unlock(calculator->mutex);
            } else {
                hits++;
            }
        }
    }

    calculator->seed = seed;

    if (!calculator->mutex) {
        double percentage = (double)hits / (double)dots;
        calculator->out_value = percentage * spanX * maxY;
    }

    return NULL;
}
