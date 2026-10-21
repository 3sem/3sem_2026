import argparse
import os
import statistics
import subprocess
import time
from itertools import chain

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def run_once(program: str, threads: int, dots: int, extra_args=None) -> float:
    cmd = [program, str(threads), str(dots)]
    if extra_args:
        cmd.extend(extra_args)
    t0 = time.perf_counter()
    res = subprocess.run(cmd, capture_output=True, text=True, check=True)
    t1 = time.perf_counter()
    try:
        return float(res.stdout.strip().splitlines()[-1])
    except (ValueError, IndexError):
        return t1 - t0


def measure(program: str, threads_list, dots: int, repeats: int, extra_args=None):
    wall = []
    for t in threads_list:
        w_runs = [run_once(program, t, dots, extra_args) for _ in range(repeats)]
        wall.append(statistics.mean(w_runs))
        print(f"  threads={t:4d}  wall={wall[-1]:8.4f} s")
    return wall


def main():
    ap = argparse.ArgumentParser(
        description="Измерение времени расчёта интеграла методом Монте-Карло от числа потоков"
    )
    ap.add_argument("--bin", default="./threads", help="путь к скомпилированному бинарнику")
    ap.add_argument("--dots", type=float, nargs="+", default=[1e7], help="число точек Монте-Карло")
    ap.add_argument("--repeats", type=int, default=5, help="число повторов для усреднения")
    ap.add_argument("--max-threads", type=int, default=None,
                    help="верхняя граница числа потоков (по умолчанию до 500)")
    ap.add_argument("--out", default="tester/threads_plot.png", help="путь для сохранения графика")
    args = ap.parse_args()

    n_cpu = os.cpu_count() or 1

    if args.max_threads is not None:
        step_mid = max(1, args.max_threads // 20)
        threads_list = sorted(set(
            list(range(1, min(20, args.max_threads + 1))) +
            list(range(20, args.max_threads + 1, step_mid))
        ))
    else:
        threads_list = list(chain(
            range(1, 20),
            range(20, 100, 10),
            range(100, 500, 100)
        ))

    out_dir = os.path.dirname(args.out)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    fig, ax = plt.subplots(figsize=(10, 6))

    for dots_f in args.dots:
        dots = int(dots_f)
        print(f"\n--- dots = {dots} ---")
        wall = measure(args.bin, threads_list, dots, args.repeats)
        ax.plot(threads_list, wall, marker="o", markersize=4,
                label=f"{dots:.0e} точек (независимые ячейки)")

    ax.axvline(n_cpu, color="red", ls="--", alpha=0.6,
               label=f"CPU cores = {n_cpu}")
    ax.grid(True, alpha=0.3)
    ax.set_xlabel("Число потоков")
    ax.set_ylabel("Wall time, s")
    ax.set_title("Зависимость реального времени выполнения от числа потоков")
    ax.legend()

    plt.tight_layout()
    plt.savefig(args.out, dpi=150)
    print(f"\nГрафик сохранён: {args.out}")


if __name__ == "__main__":
    main()
