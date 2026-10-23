#!/usr/bin/env bash
set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
export LC_ALL=C

logical_cpus=$(nproc)
physical_cores=$(lscpu -p=CORE,SOCKET | awk '!/^#/ { print }' | sort -u | wc -l)
default_max=$((logical_cpus * 8))
if ((default_max < 256)); then
    default_max=256
fi

max_threads=${1-$default_max}
repetitions=${2-3}
if [[ $# -gt 2 || ! $max_threads =~ ^[1-9][0-9]*$ ||
      ! $repetitions =~ ^[1-9][0-9]*$ ]]; then
    echo "Usage: $0 [MAX_THREADS] [REPETITIONS]" >&2
    exit 1
fi

python3 -c 'import matplotlib' || {
    echo "Install plotting dependencies: python3 -m pip install -r requirements.txt" >&2
    exit 1
}

# A separate Release build keeps debug-build settings unchanged.
cmake -S . -B build/benchmark -DCMAKE_BUILD_TYPE=Release
cmake --build build/benchmark --target benchmarkIntegral -j "$logical_cpus"

result_dir="benchmark/$(date +%Y%m%d_%H%M%S)_$$"
mkdir -p -- "$result_dir"
printf 'threads,run,seconds,integral,standard_error\n' > "$result_dir/results.csv"
printf 'expression=x*x\ninterval=[0,2]\nlogical_cpus=%s\nphysical_cores=%s\nmax_threads=%s\nrepetitions=%s\n' \
    "$logical_cpus" "$physical_cores" "$max_threads" "$repetitions" > "$result_dir/settings.txt"

# Dense measurements at small N, then powers of two and CPU-count landmarks.
thread_counts(){
    local threads
    for ((threads = 1; threads <= 16 && threads <= max_threads; threads++)); do
        echo "$threads"
    done
    for ((threads = 32; threads < max_threads; threads *= 2)); do
        echo "$threads"
    done
    for threads in "$physical_cores" "$logical_cpus" "$max_threads"; do
        if ((threads > 0 && threads <= max_threads)); then
            echo "$threads"
        fi
    done
}

mapfile -t counts < <(thread_counts | sort -nu)
echo "Expression: x*x on [0, 2]; threads: ${counts[*]}; repetitions: $repetitions"

# Warm-up is excluded from the CSV.
./build/benchmark/benchmarkIntegral 1 > /dev/null

# Alternate the direction to reduce bias from heating during a long benchmark.
for ((run = 1; run <= repetitions; run++)); do
    for ((index = 0; index < ${#counts[@]}; index++)); do
        position=$index
        if ((run % 2 == 0)); then
            position=$((${#counts[@]} - 1 - index))
        fi
        threads=${counts[position]}
        echo "Threads: $threads; run: $run/$repetitions"
        measurement=$(timeout "${BENCHMARK_TIMEOUT:-300s}" \
            ./build/benchmark/benchmarkIntegral "$threads")
        printf '%s,%s,%s\n' "$threads" "$run" "$measurement" >> "$result_dir/results.csv"
    done
done

echo "Results saved to $result_dir/results.csv"
python3 ./plotBenchmark.py "$result_dir/results.csv" \
    --output "$result_dir/thread_scaling.png" \
    --logical-cpus "$logical_cpus" --physical-cores "$physical_cores"
