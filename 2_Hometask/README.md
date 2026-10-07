## Сборка:

```bash
gcc -O2 -Wall -Wextra main.c duplex.c fileFunctions.c -o duplex
```

## Запуск тестов:

### Создание передаваемого файла:

```bash
dd if=/dev/urandom of=<input_file> bs=1048576 count=4096
```

### Запуск теста:

```bash
time ./duplex < <input_file> > <output_file>
```

### Проверка целостности:

```bash
md5sum <input_file> <output_file>
```
