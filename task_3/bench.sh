#!/usr/bin/env bash
# Прогон трёх способов IPC: прогрев, 5 замеров, md5sum, внешний time, графики.
set -euo pipefail
export LC_ALL=C

cd "$(dirname "$0")"

MODE="${1:-full}"
case "$MODE" in
    full)
        FILE_MB=256
        RUNS=5
        WARMUP=1
        ;;
    smoke)
        FILE_MB=8
        RUNS=1
        WARMUP=0
        ;;
    *)
        echo "использование: $0 [full|smoke]" >&2
        exit 2
        ;;
esac

BUFFERS=(8192 65536 1048576)
METHODS=(shm mq fifo)
IN="/dev/shm/ipc_lab_input.bin"
OUT="/dev/shm/ipc_lab_output.bin"
WANT=$((FILE_MB * 1024 * 1024))

mkdir -p run report
make -s

if command -v taskset >/dev/null 2>&1; then
    RECV=(taskset -c 2)
    SEND=(taskset -c 3)
else
    RECV=()
    SEND=()
fi

have_size=0
if [[ -f "$IN" ]]; then
    have_size=$(stat -c%s "$IN")
fi
if [[ "$have_size" != "$WANT" ]]; then
    echo "генерирую ${FILE_MB} МиБ в $IN"
    dd if=/dev/urandom of="$IN" bs=1M count="$FILE_MB" status=progress conv=fsync
fi

{
    echo "msgmax $(cat /proc/sys/kernel/msgmax)"
    echo "msgmnb $(cat /proc/sys/kernel/msgmnb)"
    echo "uname $(uname -sr)"
    echo "file_bytes $WANT"
    if ((${#RECV[@]})); then
        echo "taskset receiver cpu 2, sender cpu 3"
    else
        echo "taskset нет"
    fi
} > report/system.txt

SRC_MD5=$(md5sum "$IN" | awk '{print $1}')
echo "md5 исходного файла $SRC_MD5"

CSV=report/runs.csv
echo "method,bufsize,run,seconds,bytes,md5" > "$CSV"

CUR=""
cleanup() {
    if [[ -n "${CUR}" ]]; then
        kill "$CUR" 2>/dev/null || true
        wait "$CUR" 2>/dev/null || true
    fi
}
trap cleanup EXIT

wait_ready() {
    local err=$1 pid=$2
    local i
    for i in $(seq 1 500); do
        if grep -q '^READY$' "$err" 2>/dev/null; then
            return 0
        fi
        if ! kill -0 "$pid" 2>/dev/null; then
            echo "получатель завершился до READY" >&2
            cat "$err" >&2 || true
            return 1
        fi
        sleep 0.01
    done
    echo "таймаут ожидания READY" >&2
    cat "$err" >&2 || true
    return 1
}

run_pair() {
    local method=$1 buf=$2
    local rerr sout serr rout
    rerr=$(mktemp)
    serr=$(mktemp)
    sout=$(mktemp)
    rout=$(mktemp)

    "${RECV[@]}" "./bin/${method}_receiver" "$OUT" "$buf" >"$rout" 2>"$rerr" &
    CUR=$!
    if ! wait_ready "$rerr" "$CUR"; then
        kill "$CUR" 2>/dev/null || true
        wait "$CUR" 2>/dev/null || true
        CUR=""
        rm -f "$rerr" "$serr" "$sout" "$rout"
        return 1
    fi

    if ! "${SEND[@]}" "./bin/${method}_sender" "$IN" "$buf" >"$sout" 2>"$serr"; then
        echo "отправитель ${method} завершился с ошибкой" >&2
        cat "$serr" >&2 || true
        cat "$rerr" >&2 || true
        kill "$CUR" 2>/dev/null || true
        wait "$CUR" 2>/dev/null || true
        CUR=""
        rm -f "$rerr" "$serr" "$sout" "$rout"
        return 1
    fi

    if ! wait "$CUR"; then
        echo "получатель ${method} завершился с ошибкой" >&2
        cat "$rerr" >&2 || true
        CUR=""
        rm -f "$rerr" "$serr" "$sout" "$rout"
        return 1
    fi
    CUR=""

    if grep -q '^PIPE_CAP ' "$serr" && ! grep -q '^PIPE_CAP ' report/system.txt; then
        grep '^PIPE_CAP ' "$serr" | head -n 1 >> report/system.txt
    fi

    local sec bytes dst
    sec=$(awk '$1 == "seconds" { print $2 }' "$rout")
    bytes=$(awk '$1 == "bytes" { print $2 }' "$rout")
    if [[ -z "$sec" || -z "$bytes" ]]; then
        echo "нет строки seconds/bytes в выводе получателя" >&2
        cat "$rout" >&2 || true
        rm -f "$rerr" "$serr" "$sout" "$rout"
        return 1
    fi
    if [[ "$bytes" != "$WANT" ]]; then
        echo "размер выхода $bytes, ожидалось $WANT" >&2
        rm -f "$rerr" "$serr" "$sout" "$rout"
        return 1
    fi

    dst=$(md5sum "$OUT" | awk '{print $1}')
    if [[ "$dst" != "$SRC_MD5" ]]; then
        echo "md5 не совпал: $dst != $SRC_MD5 (${method}, buf ${buf})" >&2
        rm -f "$rerr" "$serr" "$sout" "$rout"
        return 1
    fi

    rm -f "$rerr" "$serr" "$sout" "$rout"
    printf '%s\n' "$sec"
}

for method in "${METHODS[@]}"; do
    for buf in "${BUFFERS[@]}"; do
        if [[ "$WARMUP" == 1 ]]; then
            echo "прогрев ${method} buf=${buf}"
            run_pair "$method" "$buf" >/dev/null
        fi
        for run in $(seq 1 "$RUNS"); do
            echo "замер ${method} buf=${buf} прогон ${run}/${RUNS}"
            sec=$(run_pair "$method" "$buf")
            echo "${method},${buf},${run},${sec},${WANT},ok" >> "$CSV"
            echo "  ${sec} с"
        done
    done
done

python3 plot_results.py "$CSV" report

if [[ "$MODE" == full ]]; then
    echo "пример внешнего time: shm, буфер 65536"
    rerr=$(mktemp)
    # GNU time в системе нет, поэтому встроенная команда bash.
    TIMEFORMAT=$'elapsed_sec %R\nuser_sec %U\nsys_sec %S'
    (
        time "${RECV[@]}" ./bin/shm_receiver "$OUT" 65536 >run/time_recv.out 2>"$rerr"
    ) 2>report/time_receiver.txt &
    CUR=$!
    wait_ready "$rerr" "$CUR"
    (
        time "${SEND[@]}" ./bin/shm_sender "$IN" 65536
    ) 2>report/time_sender.txt
    wait "$CUR"
    CUR=""
    rm -f "$rerr"
    dst=$(md5sum "$OUT" | awk '{print $1}')
    if [[ "$dst" != "$SRC_MD5" ]]; then
        echo "md5 внешнего прогона не совпал" >&2
        exit 1
    fi
    echo "внешний time записан в report/time_receiver.txt и report/time_sender.txt"
fi

rm -f "$IN" "$OUT" /tmp/ipc_lab_task3.fifo /tmp/ipc_lab_task3.key
echo "готово"
