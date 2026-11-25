#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
TIMEFORMAT='%R'

inputFile=${1:?Usage: bash task3-test.sh input.bin}
programs=(shm_transfer mq_transfer fifo_transfer)
sizes=(8192 65536 1048576)
inputHash=$(md5sum < "$inputFile")
mkdir -p results
outputFile=$(mktemp results/output.XXXXXX)
timeFile=$(mktemp results/time.XXXXXX)
trap 'rm -f "$outputFile" "$timeFile"' EXIT

echo 'program,buffer,run,seconds' > results/times.csv

for program in "${programs[@]}"; do
    for bufferSize in "${sizes[@]}"; do
        for run in 1 2 3; do
            echo "$program: buffer=$bufferSize, run=$run"
            if ! { time "./$program" "$bufferSize" < "$inputFile" > "$outputFile"; } 2> "$timeFile"; then
                cat "$timeFile" >&2
                exit 1
            fi
            if [[ $(md5sum < "$outputFile") != "$inputHash" ]]; then
                echo 'MD5 mismatch' >&2
                exit 1
            fi
            echo "$program,$bufferSize,$run,$(cat "$timeFile")" >> results/times.csv
        done
    done
done

python3 - "${sizes[@]}" <<'PY'
import csv
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

sizes = list(map(int, sys.argv[1:]))
with open('results/times.csv') as file:
    rows = list(csv.DictReader(file))

for program in ('shm_transfer', 'mq_transfer', 'fifo_transfer'):
    averages = []
    for size in sizes:
        times = [float(row['seconds']) for row in rows
                 if row['program'] == program and int(row['buffer']) == size]
        averages.append(sum(times) / len(times))
    plt.figure(figsize=(6, 4))
    plt.bar([f'{size // 1024} KiB' for size in sizes], averages)
    plt.title(program)
    plt.xlabel('Buffer size')
    plt.ylabel('Mean real time, seconds')
    plt.tight_layout()
    plt.savefig(f'results/{program}.png')
    plt.close()
PY

echo 'MD5 OK. Results: results/times.csv and results/*.png'

