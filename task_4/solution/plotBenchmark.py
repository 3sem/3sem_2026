#!/usr/bin/env python3
"""Plot calculation time and speedup versus thread count."""

import argparse
import csv
import math
from pathlib import Path
from statistics import median


def read_measurements(path):
    groups = {}
    with path.open(newline="", encoding="utf-8-sig") as source:
        for row in csv.DictReader(source):
            threads = int(row["threads"])
            seconds = float(row["seconds"])
            integral = float(row["integral"])
            error = float(row["standard_error"])
            if threads <= 0 or not math.isfinite(seconds) or seconds <= 0:
                raise ValueError("Thread count and elapsed time must be positive")
            if not math.isfinite(integral) or not math.isfinite(error) or error < 0:
                raise ValueError("Invalid calculation result")
            groups.setdefault(threads, []).append(seconds)
    if 1 not in groups:
        raise ValueError("CSV must include a measurement with one thread")
    return groups


def plot_measurements(groups, output, logical_cpus, physical_cores):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    threads = sorted(groups)
    times = [median(groups[count]) for count in threads]
    lower = [middle - min(groups[count]) for count, middle in zip(threads, times)]
    upper = [max(groups[count]) - middle for count, middle in zip(threads, times)]
    baseline = median(groups[1])
    speedup = [baseline / seconds for seconds in times]
    best = min(threads, key=lambda count: median(groups[count]))

    fig, axes = plt.subplots(1, 2, figsize=(13, 5))
    fig.suptitle("Масштабирование расчёта интеграла · x*x на [0, 2]", fontsize=16)
    axes[0].errorbar(threads, times, yerr=[lower, upper], fmt="o-",
                     color="#3274A1", capsize=3, label="Медиана и min–max")
    axes[0].set_ylabel("Время расчёта, с")
    axes[1].plot(threads, speedup, "o-", color="#269C89", label="T(1) / T(N)")
    axes[1].axhline(1, color="#7A828A", linestyle="--", label="Один поток")
    axes[1].set_ylabel("Ускорение, раз")

    for ax in axes:
        ax.set_xscale("log", base=2)
        ax.set_xlabel("Количество потоков N (логарифмическая шкала)")
        ax.set_ylim(bottom=0)
        ax.grid(alpha=0.25)
        ax.spines[["top", "right"]].set_visible(False)
        landmarks = [(physical_cores, "Физические ядра", "#D88432"),
                     (logical_cpus, "Доступные CPU", "#8055A2")]
        for count, label, color in landmarks:
            if count and min(threads) <= count <= max(threads):
                ax.axvline(count, color=color, linestyle=":", label=f"{label}: {count}")
        ax.legend(fontsize=8)

    fig.text(0.5, 0.025,
             f"Лучшее из измеренных: {best} потоков · "
             "Время включает создание потоков и ожидание; IPC исключён.",
             ha="center", fontsize=10)
    fig.tight_layout(rect=(0, 0.07, 1, 0.92))
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=180, facecolor="white")
    plt.close(fig)
    print(f"Best measured thread count: {best}; speedup: {baseline / median(groups[best]):.2f}x")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--logical-cpus", type=int, default=0)
    parser.add_argument("--physical-cores", type=int, default=0)
    args = parser.parse_args()
    output = args.output or args.csv.with_name("thread_scaling.png")
    try:
        groups = read_measurements(args.csv)
        plot_measurements(groups, output, args.logical_cpus, args.physical_cores)
    except (OSError, ValueError, KeyError, ImportError) as error:
        parser.exit(1, f"Cannot plot benchmark: {error}\n")
    print(f"Diagram saved to {output}")


if __name__ == "__main__":
    main()
