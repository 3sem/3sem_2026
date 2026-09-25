#!/usr/bin/env python3
"""Plot median wall-clock transfer time and observed min/max from benchmark CSV."""

import argparse
import csv
import math
from pathlib import Path
from statistics import median


MODES = {
    "fifo": ("FIFO", "#3274A1"),
    "shMem": ("Shared memory · System V", "#269C89"),
    "queue": ("Message queue · System V", "#D88432"),
}


def read_measurements(path):
    groups = {}
    file_sizes = set()
    seen = set()

    with path.open(newline="", encoding="utf-8-sig") as source:
        reader = csv.DictReader(source)
        required = {"type", "file_size", "chunk_size", "run", "real_seconds"}
        if not required.issubset(reader.fieldnames or []):
            raise ValueError("CSV must contain: " + ", ".join(sorted(required)))

        for line, row in enumerate(reader, start=2):
            try:
                mode = row["type"]
                file_size = int(row["file_size"])
                chunk_size = int(row["chunk_size"])
                run = int(row["run"])
                seconds = float(row["real_seconds"])
                if mode not in MODES or file_size <= 0 or chunk_size <= 0 or run < 0:
                    raise ValueError("invalid type, size or run")
                if not math.isfinite(seconds) or seconds < 0:
                    raise ValueError("time must be finite and non-negative")
            except (ValueError, TypeError) as error:
                raise ValueError(f"Invalid CSV row {line}: {error}") from error

            if run == 0:
                continue

            identity = (mode, file_size, chunk_size, run)
            if identity in seen:
                raise ValueError(f"Duplicate measurement in CSV row {line}")
            seen.add(identity)
            file_sizes.add(file_size)
            groups.setdefault((mode, chunk_size), []).append(seconds)

    if not groups:
        raise ValueError("CSV contains no measured runs")
    if len(file_sizes) != 1:
        raise ValueError("Use results for one fixed file size per diagram")

    return file_sizes.pop(), groups


def format_bytes(size):
    for unit, factor in (("ГиБ", 1024 ** 3), ("МиБ", 1024 ** 2), ("КиБ", 1024)):
        if size % factor == 0:
            return f"{size // factor} {unit}"
    return f"{size} Б"


def plot_measurements(file_size, groups, output):
    import matplotlib

    matplotlib.use("Agg")  # Works on Linux without a graphical desktop.
    import matplotlib.pyplot as plt

    chunks = sorted({chunk for _, chunk in groups})
    maximum = max(max(values) for values in groups.values())
    y_limit = maximum * 1.3 if maximum > 0 else 0.01

    figure_width = max(13, len(chunks) * 3)
    fig, axes = plt.subplots(1, 3, figsize=(figure_width, 5.5), sharey=True)
    fig.suptitle(f"Время передачи файла · {format_bytes(file_size)}", fontsize=18, y=0.96)

    for ax, (mode, (title, color)) in zip(axes, MODES.items()):
        for index, chunk in enumerate(chunks):
            values = groups.get((mode, chunk))
            if not values:
                ax.text(index, y_limit * 0.04, "нет данных", ha="center", fontsize=9)
                continue

            middle = median(values)
            lower = middle - min(values)
            upper = max(values) - middle
            ax.bar(index, middle, width=0.58, color=color, zorder=3)
            ax.errorbar(index, middle, yerr=[[lower], [upper]], fmt="none",
                        ecolor="#28323C", capsize=5, linewidth=1.3, zorder=4)
            ax.text(index, max(values) + y_limit * 0.025,
                    f"{middle:.3f}\nn={len(values)}", ha="center", va="bottom", fontsize=9)

        ax.set_title(title, fontsize=12, pad=12)
        ax.set_xticks(range(len(chunks)), [format_bytes(chunk) for chunk in chunks])
        ax.set_xlabel("Размер порции", labelpad=10)
        ax.set_xlim(-0.6, len(chunks) - 0.4)
        ax.set_ylim(0, y_limit)
        ax.grid(axis="y", color="#E2E6EA", zorder=0)
        ax.spines[["top", "right"]].set_visible(False)

    axes[0].set_ylabel("Время real, с")
    fig.text(0.5, 0.06,
             "Столбцы — медиана; интервалы — минимум–максимум; n — число повторений.",
             ha="center", fontsize=10)
    fig.text(0.5, 0.025,
             "Время включает запуск процессов, синхронизацию и файловый ввод-вывод.",
             ha="center", fontsize=9, color="#59636D")
    fig.tight_layout(rect=(0, 0.11, 1, 0.91))

    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=180, facecolor="white")
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    default_csv = Path(__file__).resolve().parent / "benchmark" / "results.csv"
    parser.add_argument("csv", nargs="?", type=Path, default=default_csv)
    parser.add_argument("--output", type=Path, help="Output PNG path")
    args = parser.parse_args()
    output = args.output or args.csv.with_name("transfer_times.png")

    try:
        file_size, groups = read_measurements(args.csv)
        plot_measurements(file_size, groups, output)
    except ImportError:
        parser.exit(1, "Install plotting dependencies: python3 -m pip install -r requirements.txt\n")
    except (OSError, ValueError) as error:
        parser.exit(1, f"Cannot plot benchmark: {error}\n")

    print(f"Diagram saved to {output}")


if __name__ == "__main__":
    main()
