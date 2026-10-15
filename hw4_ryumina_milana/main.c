#include "monte_carlo.h"

int main(int argc, char **argv) {
    int num_threads = 1;
    int mode = MODE_INDEPENDENT;

    if (argc > 1) {
        num_threads = atoi(argv[1]);
    } else {
        printf("Введите число потоков: ");
        if (scanf("%d", &num_threads) != 1) {
            fprintf(stderr, "Ошибка ввода\n");
            return 1;
        }
    }

    if (argc > 2) {
        mode = atoi(argv[2]);
    }

    if (num_threads < 1 || num_threads > MAX_THREADS) {
        fprintf(stderr, "Число потоков должно быть от 1 до %d\n", MAX_THREADS);
        return 1;
    }

    const char *mode_name = (mode == MODE_INDEPENDENT) ? "INDEPENDENT" : "SHARED";
    printf("Threads: %d, Mode: %s\n", num_threads, mode_name);

    double result = CalculateIntegral(ExponentialFunc, num_threads, mode,
                                      0.0, 1.0, 0.0, ExponentialFunc(1.0));

    printf("Итоговый результат: %lg\n", result);
    return 0;
}