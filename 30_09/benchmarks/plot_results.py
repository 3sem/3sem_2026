#!/usr/bin/env python3
"""Plot elapsed time and speedup from Monte Carlo experiments."""

import argparse
import csv
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

MODE_LABELS = {
    "per_thread": "Independent cell per thread",
    "shared_mutex": "Shared counter + mutex",
}


def load_samples(path):
    """Load elapsed-time samples grouped by mode and thread count."""
    samples = defaultdict(list)
    with path.open(newline="", encoding="utf-8") as file:
        reader = csv.DictReader(file)
        required = {"mode", "threads", "elapsed_seconds"}
        missing = required - set(reader.fieldnames or [])
        if missing:
            raise ValueError(
                "CSV is missing required columns: " + ", ".join(sorted(missing))
            )

        for line_number, row in enumerate(reader, start=2):
            try:
                mode = row["mode"]
                threads = int(row["threads"])
                elapsed = float(row["elapsed_seconds"])
            except (TypeError, ValueError) as error:
                raise ValueError(f"Invalid CSV row {line_number}: {error}") from error

            if mode not in MODE_LABELS:
                raise ValueError(f"Unknown mode {mode!r} on row {line_number}")
            if threads < 1:
                raise ValueError(f"Thread count must be positive on row {line_number}")
            if elapsed <= 0:
                raise ValueError(f"Elapsed time must be positive on row {line_number}")

            samples[(mode, threads)].append(elapsed)

    if not samples:
        raise ValueError("CSV has no measurements")
    return samples


def configure_thread_axis(axis, thread_counts):
    """Use log2 spacing while labelling ticks with actual tested thread counts."""
    axis.set_xscale("log", base=2)
    axis.set_xticks(thread_counts, labels=[str(count) for count in thread_counts])
    axis.set_xlabel("Number of threads")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", type=Path, default=Path("results.csv"))
    args = parser.parse_args()

    samples = load_samples(args.csv)
    modes = ("per_thread", "shared_mutex")
    thread_counts = sorted({threads for _, threads in samples})
    medians = {key: statistics.median(values) for key, values in samples.items()}

    # Plot elapsed time.
    fig, axis = plt.subplots(figsize=(9.5, 5.8))
    for mode in modes:
        xs = [n for n in thread_counts if (mode, n) in medians]
        ys = [medians[(mode, n)] for n in xs]
        if xs:
            axis.plot(xs, ys, marker="o", label=MODE_LABELS[mode])

    configure_thread_axis(axis, thread_counts)
    axis.set_ylabel("Median elapsed time (seconds)")
    axis.set_title("Monte Carlo: elapsed time vs. thread count")
    axis.grid(True, which="both", linestyle="--", alpha=0.4)
    axis.legend()
    fig.tight_layout()
    time_path = args.csv.parent / "thread_scaling_time.png"
    fig.savefig(time_path, dpi=160)
    plt.close(fig)

    # Plot speedup. Each mode is normalized to its own one-thread median.
    fig, axis = plt.subplots(figsize=(9.5, 5.8))
    plotted_modes = 0
    for mode in modes:
        baseline = medians.get((mode, 1))
        if baseline is None:
            print(f"Warning: no one-thread measurement for {mode}; skipping speedup")
            continue

        xs = [n for n in thread_counts if (mode, n) in medians]
        ys = [baseline / medians[(mode, n)] for n in xs]
        axis.plot(xs, ys, marker="o", label=MODE_LABELS[mode])
        plotted_modes += 1

    configure_thread_axis(axis, thread_counts)
    axis.set_ylabel("Speedup S(n) = T(1) / T(n)")
    axis.set_title("Monte Carlo: parallel speedup")
    axis.grid(True, which="both", linestyle="--", alpha=0.4)
    axis.legend()
    fig.tight_layout()
    speedup_path = args.csv.parent / "thread_scaling_speedup.png"
    fig.savefig(speedup_path, dpi=160)
    plt.close(fig)

    print(f"Saved {time_path}")
    print(f"Saved {speedup_path}")
    if plotted_modes == 0:
        print("Warning: speedup plot contains only the ideal reference line")


if __name__ == "__main__":
    main()

