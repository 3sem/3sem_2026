"""Measure all 3 IPC methods with 3 buffer sizes and verify every output MD5."""

import argparse
import csv
from datetime import datetime, timezone
import itertools
import json
import os
from pathlib import Path
import platform
import random
import statistics
import tempfile

from support import (METHODS, PROJECT_DIR, checksumTool, fileMd5, parseFields,
                     readLimits, runCommand, transferFile)


def createInput(path, size):
    with path.open("wb") as stream:
        remaining = size
        while remaining:
            count = min(1024 * 1024, remaining)
            stream.write(os.urandom(count))
            remaining -= count


def summarizeRows(rows, buffer_sizes):
    summary = []
    for method, buffer_size in itertools.product(METHODS, buffer_sizes):
        values = [row["seconds"] for row in rows
                  if row["method"] == method and row["buffer_bytes"] == buffer_size]
        summary.append({"method": method, "buffer_bytes": buffer_size,
                        "median_seconds": statistics.median(values),
                        "min_seconds": min(values), "max_seconds": max(values),
                        "repeats": len(values)})
    return summary


def drawPlots(summary, buffer_sizes, output_dir, file_size):
    # No GUI and no writes to the user's global Matplotlib cache.
    os.environ.setdefault("MPLCONFIGDIR", str(output_dir / ".matplotlib"))
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    names = {"shm": "Разделяемая память SYS V", "mq": "Очередь сообщений SYS V",
             "fifo": "Именованный канал FIFO"}
    colors = {"shm": "#2763B3", "mq": "#D27C24", "fifo": "#25826D"}
    labels = [f"{size / 1024:g} КиБ" for size in buffer_sizes]
    for method in METHODS:
        data = [row for row in summary if row["method"] == method]
        medians = [row["median_seconds"] for row in data]
        errors = [[row["median_seconds"] - row["min_seconds"] for row in data],
                  [row["max_seconds"] - row["median_seconds"] for row in data]]
        figure, axis = plt.subplots(figsize=(8, 5), layout="constrained")
        bars = axis.bar(labels, medians, color=colors[method], width=0.58,
                        yerr=errors, capsize=6)
        axis.bar_label(bars, labels=[f"{value:.4f} с" for value in medians], padding=8)
        axis.set(title=names[method], xlabel="Размер пользовательского буфера",
                 ylabel="Время передачи, с", ylim=(0, max(row["max_seconds"] for row in data) * 1.23))
        axis.set_axisbelow(True)
        axis.grid(axis="y", alpha=0.22)
        figure.suptitle(f"Файл {file_size:,} байт • медиана {data[0]['repeats']} повторов\n"
                        "Усы: минимум и максимум; MD5 совпал во всех запусках", fontsize=10)
        figure.savefig(output_dir / f"{method}.png", dpi=180)
        figure.savefig(output_dir / f"{method}.pdf")
        plt.close(figure)

    figure, axis = plt.subplots(figsize=(9, 5), layout="constrained")
    for method_index, method in enumerate(METHODS):
        data = [row for row in summary if row["method"] == method]
        positions = [index + (method_index - 1) * 0.25 for index in range(3)]
        axis.bar(positions, [row["median_seconds"] for row in data], width=0.24,
                 label=names[method], color=colors[method])
    axis.set(xticks=range(3), xticklabels=labels, xlabel="Размер пользовательского буфера",
             ylabel="Медианное время передачи, с", title="Сравнение трёх способов IPC")
    axis.set_ylim(bottom=0)
    axis.set_axisbelow(True)
    axis.grid(axis="y", alpha=0.22)
    axis.legend()
    figure.savefig(output_dir / "comparison.png", dpi=180)
    plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, help="existing regular input file")
    parser.add_argument("--size-mib", type=int, default=256)
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--buffers", nargs=3, type=int, help="three increasing sizes, in bytes")
    parser.add_argument("--seed", type=int, default=20261021, help="shuffle seed")
    parser.add_argument("--output-dir", type=Path, default=PROJECT_DIR / "results")
    arguments = parser.parse_args()
    if arguments.size_mib < 1 or arguments.repeats < 2 or arguments.warmups < 0:
        parser.error("size-mib >= 1, repeats >= 2 and warmups >= 0 are required")
    # Check plotting dependencies before starting a long experiment.
    import importlib.util
    if importlib.util.find_spec("matplotlib") is None:
        parser.error("install matplotlib first: python3 -m pip install matplotlib")
    output_dir = arguments.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    limits = readLimits()
    buffer_sizes = arguments.buffers or [limits["message_limit"], limits["fifo_capacity"],
                                         max(1024 * 1024, 16 * limits["fifo_capacity"])]
    if (buffer_sizes != sorted(set(buffer_sizes)) or buffer_sizes[0] < 1 or
            buffer_sizes[-1] > 16 * 1024 * 1024):
        parser.error("need three distinct increasing buffer sizes within 1..16777216; use --buffers")
    combinations = list(itertools.product(METHODS, buffer_sizes))
    random_order = random.Random(arguments.seed)
    rows = []
    with tempfile.TemporaryDirectory(prefix="task3_benchmark_") as directory:
        temporary_dir = Path(directory)
        source = arguments.input.resolve() if arguments.input else temporary_dir / "input.bin"
        if arguments.input is None:
            createInput(source, arguments.size_mib * 1024 * 1024 + 123)
        if not source.is_file():
            parser.error("input must be a regular file")
        file_size = source.stat().st_size
        input_md5 = fileMd5(source)
        output = temporary_dir / "output.bin"
        metadata = {
            "timestamp_utc": datetime.now(timezone.utc).isoformat(),
            "system": platform.system(), "release": platform.release(),
            "machine": platform.machine(), "macos_version": platform.mac_ver()[0],
            "cpu_count": os.cpu_count(), "python": platform.python_version(),
            "compiler": runCommand(["cc", "--version"]).stdout.splitlines()[0],
            "limits": limits, "buffer_sizes": buffer_sizes, "file_bytes": file_size,
            "input_md5": input_md5, "checksum_tool": checksumTool(),
            "input_kind": "user supplied" if arguments.input else "os.urandom, non-sparse",
            "repeats": arguments.repeats, "warmups_per_combination": arguments.warmups,
            "shuffle_seed": arguments.seed,
            "timing": "CLOCK_MONOTONIC: before fork through receiver close and waitpid; no fsync",
            "cache_policy": "warm input; MD5 and generation outside timing; no cache flush",
            "storage": "temporary directory on the host filesystem",
        }
        (output_dir / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
        print(f"Limits: {limits}; buffers: {buffer_sizes}; input MD5: {input_md5}", flush=True)
        with (output_dir / "measurements.csv").open("w", newline="") as stream:
            fieldnames = ["round", "order", "method", "buffer_bytes", "file_bytes",
                          "seconds", "input_md5", "output_md5"]
            writer = csv.DictWriter(stream, fieldnames=fieldnames)
            writer.writeheader()
            order = 0
            for round_index in range(-arguments.warmups, arguments.repeats):
                random_order.shuffle(combinations)
                for method, buffer_size in combinations:
                    result = transferFile(method, source, output, buffer_size, timeout=600)
                    if result.returncode != 0:
                        raise RuntimeError(f"{method}/{buffer_size}: {result.stderr}")
                    fields = parseFields(result.stdout)
                    output_md5 = fileMd5(output)
                    if (output_md5 != input_md5 or output.stat().st_size != file_size or
                            int(fields["bytes"]) != file_size or fields["method"] != method or
                            int(fields["buffer"]) != buffer_size):
                        raise RuntimeError(f"Integrity check failed for {method}/{buffer_size}")
                    elapsed = float(fields["seconds"])
                    label = "warmup" if round_index < 0 else f"round {round_index + 1}"
                    print(f"{label}: {method:4} buffer={buffer_size:7} {elapsed:.6f} s MD5 OK", flush=True)
                    if round_index >= 0:
                        order += 1
                        row = dict(round=round_index + 1, order=order, method=method,
                                   buffer_bytes=buffer_size, file_bytes=file_size, seconds=elapsed,
                                   input_md5=input_md5, output_md5=output_md5)
                        rows.append(row)
                        writer.writerow(row)
                        stream.flush()
            if fileMd5(source) != input_md5:
                raise RuntimeError("Input file changed during the experiment")
        summary = summarizeRows(rows, buffer_sizes)
        (output_dir / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
        drawPlots(summary, buffer_sizes, output_dir, file_size)
        print(f"Saved {len(rows)} measurements and plots to {output_dir}")


if __name__ == "__main__":
    main()
