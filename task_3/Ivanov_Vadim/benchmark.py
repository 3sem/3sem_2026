import argparse
from contextlib import ExitStack
import csv
import fcntl
import hashlib
import html
import os
from pathlib import Path
import random
import shutil
import statistics
import subprocess
import tempfile


def md5(path):
    digest = hashlib.md5()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def buffer_sizes():
    small = int(Path("/proc/sys/kernel/msgmax").read_text().strip())
    read_end, write_end = os.pipe()
    try:
        medium = fcntl.fcntl(write_end, fcntl.F_GETPIPE_SZ)
    finally:
        os.close(read_end)
        os.close(write_end)
    if medium <= small:
        medium = 4 * small
    return [small, medium, max(1024 * 1024, 16 * medium)]


def draw_chart(path, title, rows):
    width, height = 640, 400
    maximum = max(row["seconds"] for row in rows) * 1.15 or 1.0
    elements = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="320" y="30" text-anchor="middle" font-size="20">{html.escape(title)}</text>',
        '<path d="M70 60V325H600" stroke="black" fill="none"/>',
        '<text x="15" y="55" font-size="14">seconds</text>',
    ]
    for index, row in enumerate(rows):
        position = 120 + index * 170
        bar_height = row["seconds"] / maximum * 240
        elements.append(
            f'<rect x="{position}" y="{325 - bar_height:.2f}" width="90" '
            f'height="{bar_height:.2f}" fill="#4588bb"/>'
        )
        elements.append(
            f'<text x="{position + 45}" y="{315 - bar_height:.2f}" '
            f'text-anchor="middle" font-size="14">{row["seconds"]:.6f}</text>'
        )
        elements.append(
            f'<text x="{position + 45}" y="350" text-anchor="middle" '
            f'font-size="14">{row["buffer_bytes"]} B</text>'
        )
    elements.append('</svg>')
    path.write_text("\n".join(elements), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Compare SYS V shared memory, message queues and FIFO")
    parser.add_argument("source", type=Path)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--order-seed", type=int, default=20261021)
    parser.add_argument("--memory-files", action="store_true",
                        help="copy source to an anonymous memory file and keep output in memory")
    parser.add_argument("--output", type=Path, default=Path("results"))
    arguments = parser.parse_args()
    if arguments.repeats < 1:
        parser.error("--repeats must be positive")
    if arguments.warmups < 0:
        parser.error("--warmups must not be negative")
    if arguments.memory_files and not hasattr(os, "memfd_create"):
        parser.error("--memory-files requires Linux with os.memfd_create")
    source = arguments.source.resolve(strict=True)
    directory = Path(__file__).resolve().parent
    arguments.output.mkdir(parents=True, exist_ok=True)
    expected = md5(source)
    sizes = buffer_sizes()
    print(f"Buffer sizes: {sizes}; repetitions: {arguments.repeats}; "
          f"warmups: {arguments.warmups}; order seed: {arguments.order_seed}", flush=True)
    print(f"Storage mode: {'memory files (memfd)' if arguments.memory_files else 'filesystem files'}",
          flush=True)
    programs = ("shm_transfer", "mq_transfer", "fifo_transfer")
    configurations = [(program, size) for program in programs for size in sizes]
    elapsed = {configuration: [] for configuration in configurations}
    generator = random.Random(arguments.order_seed)
    records = []
    summary = []
    with ExitStack() as resources:
        pass_fds = ()
        transfer_source = source
        if arguments.memory_files:
            source_fd = os.memfd_create("ipc_benchmark_source")
            resources.callback(os.close, source_fd)
            destination_fd = os.memfd_create("ipc_benchmark_destination")
            resources.callback(os.close, destination_fd)
            transfer_source = Path(f"/proc/self/fd/{source_fd}")
            destination = Path(f"/proc/self/fd/{destination_fd}")
            pass_fds = (source_fd, destination_fd)
            with source.open("rb") as original, transfer_source.open("wb") as memory:
                shutil.copyfileobj(original, memory, length=1024 * 1024)
        else:
            temporary = resources.enter_context(
                tempfile.TemporaryDirectory(prefix="ipc_benchmark_", dir=arguments.output))
            destination = Path(temporary).resolve() / "received.bin"

        def run_transfer(program, size):
            completed = subprocess.run(
                [str(directory / program), str(transfer_source), str(destination), str(size)],
                check=True, text=True, stdout=subprocess.PIPE, pass_fds=pass_fds,
            )
            seconds = float(completed.stdout.strip().split("=")[-1])
            actual = md5(destination)
            if actual != expected:
                raise RuntimeError(f"MD5 mismatch: {program}, buffer {size}")
            return seconds, actual

        for program, size in configurations:
            for warmup in range(1, arguments.warmups + 1):
                run_transfer(program, size)
                print(f"Warmup {warmup}: {program:14} {size:9} B  MD5 OK", flush=True)

        for repeat in range(1, arguments.repeats + 1):
            order = configurations.copy()
            generator.shuffle(order)
            for program, size in order:
                seconds, actual = run_transfer(program, size)
                elapsed[program, size].append(seconds)
                records.append({
                    "program": program, "buffer_bytes": size, "repeat": repeat,
                    "run_order": len(records) + 1, "seconds": seconds, "md5": actual,
                })
                print(f"Run {len(records):2d}: {program:14} {size:9} B  "
                      f"{seconds:.6f} s  MD5 OK", flush=True)

        for program in programs:
            chart_rows = []
            for size in sizes:
                durations = elapsed[program, size]
                row = {"program": program, "buffer_bytes": size,
                       "seconds": statistics.median(durations),
                       "min_seconds": min(durations), "max_seconds": max(durations),
                       "repeats": len(durations)}
                chart_rows.append(row)
                summary.append(row)
                print(f"{program:14} {size:9} B  median {row['seconds']:.6f} s", flush=True)
            draw_chart(arguments.output / f"{program}.svg", program, chart_rows)
            if chart_rows[0]["seconds"] > 0:
                ratio = chart_rows[-1]["seconds"] / chart_rows[0]["seconds"]
                print(f"  Large/small buffer time ratio: {ratio:.3f}", flush=True)
    with (arguments.output / "measurements.csv").open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=["program", "buffer_bytes", "repeat", "run_order", "seconds", "md5"])
        writer.writeheader()
        writer.writerows(records)
    with (arguments.output / "medians.csv").open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=["program", "buffer_bytes", "seconds", "min_seconds", "max_seconds", "repeats"])
        writer.writeheader()
        writer.writerows(summary)
    best = min(summary, key=lambda row: row["seconds"])
    print(f"Fastest measured: {best['program']}, {best['buffer_bytes']} B, {best['seconds']:.6f} s")
    if arguments.memory_files:
        print("Times include transfer and memory-file writes; source copying and MD5 are excluded.")
    else:
        print("Times include transfer and destination writes through the page cache, without fsync.")


if __name__ == "__main__":
    main()
