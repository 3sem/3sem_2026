#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(cd -- "$SCRIPT_DIR/.." && pwd)
cd "$SCRIPT_DIR"

for command_name in lscpu sort shuf awk python3; do
    command -v "$command_name" >/dev/null 2>&1 || {
        echo "Required command not found: $command_name" >&2
        exit 1
    }
done

PROGRAM_A=${PROGRAM_A:-"$PROJECT_DIR/build/monte_carlo_a"}
PROGRAM_B=${PROGRAM_B:-"$PROJECT_DIR/build/monte_carlo_b"}

if [[ ! -x "$PROGRAM_A" || ! -x "$PROGRAM_B" ]]; then
    echo "Executables not found. Build with CMake first." >&2
    echo "For experiments, configure using:" >&2
    echo "cmake -S . -B build-benchmarks -DBUILD_BENCHMARKS=ON" >&2
    exit 1
fi

POINTS=${POINTS:-2000000}
REPEATS=${REPEATS:-3}

if [[ ! "$POINTS" =~ ^[1-9][0-9]*$ ||
      ! "$REPEATS" =~ ^[1-9][0-9]*$ ]]; then
    echo "POINTS and REPEATS must be positive integers" >&2
    exit 1
fi

PHYSICAL_CORES=$(
    lscpu -p=CORE,SOCKET |
        awk -F, '$0 !~ /^#/ { print $1 "," $2 }' |
        sort -u |
        wc -l
)

if [[ ! "$PHYSICAL_CORES" =~ ^[1-9][0-9]*$ ]]; then
    echo "Could not determine physical core count using lscpu" >&2
    exit 1
fi

if [[ -n "${THREAD_COUNTS:-}" ]]; then
    read -r -a THREADS <<< "$THREAD_COUNTS"
else
    THREADS=()

    # Подробно исследуем диапазон от 1 до 32 потоков.
    for ((n = 1; n <= 32; n += 3)); do
        THREADS+=("$n")
    done

    # Затем исследуем степени двойки вплоть до 2^17.
    MAX_EXPONENT=17

    for ((exponent = 6; exponent <= MAX_EXPONENT; ++exponent)); do
        THREADS+=("$((1 << exponent))")
    done
fi

for threads in "${THREADS[@]}"; do
    if [[ ! "$threads" =~ ^[1-9][0-9]*$ ]]; then
        echo "Invalid thread count: $threads" >&2
        exit 1
    fi

    if (( threads > POINTS )); then
        echo "Thread count $threads exceeds POINTS=$POINTS" >&2
        exit 1
    fi
done

mapfile -t THREADS < <(printf '%s\n' "${THREADS[@]}" | sort -nu)

CSV_FILE="$SCRIPT_DIR/results.csv"
printf '%s\n' \
    'mode,threads,points,run,elapsed_seconds,hits,estimate,absolute_error' \
    > "$CSV_FILE"

SCENARIOS=()

for mode in per_thread shared_mutex; do
    for threads in "${THREADS[@]}"; do
        SCENARIOS+=("$mode,$threads")
    done
done

echo "Physical cores: $PHYSICAL_CORES"
echo "Thread counts: ${THREADS[*]}"
echo "Points per run: $POINTS; repetitions: $REPEATS"
echo "IPC key file: $PROJECT_DIR/monte_carlo.key"

for ((run = 1; run <= REPEATS; ++run)); do
    echo
    echo "Measurement round $run/$REPEATS"

    mapfile -t ORDERED_SCENARIOS < <(
        printf '%s\n' "${SCENARIOS[@]}" | shuf
    )

    for scenario in "${ORDERED_SCENARIOS[@]}"; do
        IFS=, read -r mode threads <<< "$scenario"

        echo "Running mode=$mode, threads=$threads"

        # Remove stale shared memory from a previous interrupted run.
        (
            cd "$PROJECT_DIR"
            "$PROGRAM_B" --cleanup
        ) >/dev/null

        # Run program A from the project root so both programs use
        # the same monte_carlo.key file.
        time_output=$(
            cd "$PROJECT_DIR"
            "$PROGRAM_A" "$POINTS" "$threads" "$mode"
        )

        # Read and process the results from the same shared-memory segment.
        result_output=$(
            cd "$PROJECT_DIR"
            "$PROGRAM_B"
        )

        echo "$time_output"
        echo "$result_output"

        IFS=, read -r \
            time_tag time_mode time_threads time_points elapsed \
            <<< "$time_output"

        IFS=, read -r \
            result_tag result_mode result_threads result_points \
            hits estimate absolute_error \
            <<< "$result_output"

        if [[ "$time_tag" != TIME ||
              "$result_tag" != RESULT ||
              "$time_mode" != "$mode" ||
              "$result_mode" != "$mode" ||
              "$time_threads" != "$threads" ||
              "$result_threads" != "$threads" ||
              "$time_points" != "$POINTS" ||
              "$result_points" != "$POINTS" ]]; then
            echo "Unexpected output from A or B" >&2
            exit 1
        fi

        printf '%s,%s,%s,%s,%s,%s,%s,%s\n' \
            "$mode" \
            "$threads" \
            "$POINTS" \
            "$run" \
            "$elapsed" \
            "$hits" \
            "$estimate" \
            "$absolute_error" \
            >> "$CSV_FILE"
    done
done

echo
echo "Measurements saved to: $CSV_FILE"

if python3 -c 'import matplotlib' >/dev/null 2>&1; then
    python3 "$SCRIPT_DIR/plot_results.py" --csv "$CSV_FILE"
else
    echo "matplotlib is not installed; skipping plots."
    echo "Install it using your system package manager."
fi
