# Интегрирование методом Монте-Карло

Программа вычисляет интеграл `sin(x)` на отрезке `[0, 1]` c помощью метода Монте-Карло на <num_of_threads> потоках.

## Сборка

Нужны Linux и GCC. Все команды выполняются из папки с исходниками.

```sh
gcc -std=c17 -O2 -Wall -Wextra generator.c generatorFuncs.c -o generator -pthread -lm -lrt
gcc -std=c17 -O2 -Wall -Wextra integrator.c integratorFuncs.c -o integrator -lm -lrt
```

## Запуск

```sh
./integrator <num_of_threads>
```
