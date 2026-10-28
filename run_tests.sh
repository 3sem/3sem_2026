#!/bin/bash
# Сборка трёх программ, прогон каждого с тремя размерами буфера,
# замер времени, проверка md5sum, сбор результатов в CSV.

set -e

SIZES=(8192 65536 1048576)
SIZE_NAMES=("8K" "64K" "1M")
TOOLS=("fifo" "msg" "shm")

echo ">>> Компиляция..."
gcc -Wall -Wextra fifo.c -o fifo
gcc -Wall -Wextra msg.c  -o msg
gcc -Wall -Wextra shm.c  -o shm
echo "    OK"

if [ ! -f input.txt ]; then
    echo ">>> Создаю input.txt (100 МБ)..."
    head -c 100M /dev/urandom > input.txt
fi

echo "tool,size,run,time_sec,md5_ok" > results.csv

for i in "${!TOOLS[@]}"; do
    tool="${TOOLS[$i]}"
    for j in "${!SIZES[@]}"; do
        size="${SIZES[$j]}"
        sname="${SIZE_NAMES[$j]}"
        for run in 1 2 3; do
            out="out_${tool}_${sname}.txt"

            line=$(./$tool input.txt "$out" "$size" 2>&1 >/dev/null)
            t=$(echo "$line" | sed -n 's/.*time=\([0-9.]*\).*/\1/p')

            if cmp -s input.txt "$out"; then
                md5="ok"
            else
                md5="FAIL"
            fi

            echo "$tool,$sname,$run,$t,$md5"
            echo "$tool,$sname,$run,$t,$md5" >> results.csv
        done
    done
done

echo
echo ">>> Готово. Результаты в results.csv"
echo ">>> Все выходные файлы начинаются с out_*"

echo
echo ">>> Итоговый md5sum (все должны совпасть):"
md5sum input.txt out_*.txt | sort