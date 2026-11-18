import argparse
import csv
import os
from pathlib import Path
import random
import statistics
import subprocess


def physical_cores():
    available = os.sched_getaffinity(0) if hasattr(os, "sched_getaffinity") else None
    cores = set()
    for directory in Path("/sys/devices/system/cpu").glob("cpu[0-9]*"):
        if available is not None and int(directory.name[3:]) not in available:
            continue
        try:
            topology = directory / "topology"
            cores.add(((topology / "physical_package_id").read_text().strip(),
                       (topology / "core_id").read_text().strip()))
        except OSError:
            pass
    return len(cores) or len(available or []) or os.cpu_count() or 1


def write_chart(rows, destination):
    maximum = max(row["seconds"] for row in rows) or 1
    width = max(700, 85 * len(rows) + 100)
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="430">',
             '<rect width="100%" height="100%" fill="white"/>',
             '<g font-family="sans-serif" font-size="13" fill="#222">',
             '<text x="50" y="30">Monte Carlo: time vs. thread count</text>',
             '<text x="50" y="55">Seconds; median of repeated runs. Lower is better.</text>',
             f'<path d="M50 75 V350 H{width - 20}" fill="none" stroke="#777"/>']
    step = (width - 90) / len(rows)
    for index, row in enumerate(rows):
        left = 60 + index * step
        height = row["seconds"] / maximum * 240
        parts.append(f'<rect x="{left:.1f}" y="{350 - height:.1f}" '
                     f'width="{step * .65:.1f}" height="{height:.1f}" fill="#3784b5"/>')
        parts.append(f'<text x="{left:.1f}" y="{340 - height:.1f}">{row["seconds"]:.4f}</text>')
        parts.append(f'<text x="{left:.1f}" y="375">{row["threads"]}</text>')
    parts.extend(['<text x="50" y="405">Number of threads</text>', '</g></svg>'])
    destination.write_text("\n".join(parts), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--points", type=int, default=100_000_000)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--order-seed", type=int, default=20261021)
    parser.add_argument("--threads", type=int, nargs="+")
    parser.add_argument("--output", type=Path, default=Path("measurements"))
    arguments = parser.parse_args()
    cores = physical_cores()
    logical = len(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else os.cpu_count() or 1
    counts = {1, cores, logical, cores * 2, cores * 4}
    power = 2
    while power < cores * 4:
        counts.add(power)
        power *= 2
    counts = sorted(set(arguments.threads)) if arguments.threads else sorted(counts)
    if arguments.repeats < 1 or arguments.points < 1:
        parser.error("points and repeats must be positive")
    if arguments.warmups < 0:
        parser.error("warmups must be nonnegative")
    if not counts or min(counts) < 1 or max(counts) > min(4096, arguments.points):
        parser.error("each thread count must be in 1..min(4096, points)")
    if 1 not in counts:
        counts.insert(0, 1)
    directory = Path(__file__).resolve().parent
    arguments.output.mkdir(parents=True, exist_ok=True)
    rows = []
    raw_rows = []
    print(f"Physical cores: {cores}; available logical CPUs: {logical}", flush=True)
    print(f"Points: {arguments.points}; warmups: {arguments.warmups}; "
          f"repeats: {arguments.repeats}; order seed: {arguments.order_seed}", flush=True)

    def run_once(count, seed, label):
        name = f"/kt_mc_{os.getpid()}_{count}_{label}"
        try:
            subprocess.run([str(directory / "program_a"), str(count), str(arguments.points),
                            name, str(seed)], check=True, capture_output=True, text=True)
            completed = subprocess.run([str(directory / "program_b"), name], check=True,
                                       capture_output=True, text=True)
        finally:
            Path("/dev/shm", name[1:]).unlink(missing_ok=True)
        return dict(line.split("=", 1) for line in completed.stdout.splitlines())

    for warmup in range(arguments.warmups):
        for count in counts:
            run_once(count, 1_000_000 + warmup, f"warmup_{warmup}")
            print(f"warmup={warmup + 1} threads={count:4d} done", flush=True)

    generator = random.Random(arguments.order_seed)
    durations = {count: [] for count in counts}
    run_order = 0
    for repeat in range(arguments.repeats):
        order = counts.copy()
        generator.shuffle(order)
        seed = 12345 + repeat
        for count in order:
            run_order += 1
            values = run_once(count, seed, f"run_{run_order}")
            seconds = float(values["seconds"])
            durations[count].append(seconds)
            raw_rows.append({"threads": count, "repeat": repeat + 1, "run_order": run_order,
                             "seed": seed, "seconds": seconds,
                             "integral": values["integral"], "error": values["error"]})
            print(f"repeat={repeat + 1} run={run_order:3d} threads={count:4d} "
                  f"time={seconds:.6f} s", flush=True)
    for count in counts:
        samples = durations[count]
        rows.append({"threads": count, "seconds": statistics.median(samples),
                     "min_seconds": min(samples), "max_seconds": max(samples),
                     "repeats": len(samples)})
        print(f"threads={count:4d} median={rows[-1]['seconds']:.6f} s", flush=True)
    baseline = rows[0]["seconds"]
    for row in rows:
        row["speedup"] = baseline / row["seconds"] if row["seconds"] else 0
    for filename, data in (("results.csv", rows), ("runs.csv", raw_rows)):
        with (arguments.output / filename).open("w", newline="", encoding="utf-8") as stream:
            writer = csv.DictWriter(stream, fieldnames=list(data[0]))
            writer.writeheader()
            writer.writerows(data)
    write_chart(rows, arguments.output / "time.svg")
    fastest = min(rows, key=lambda row: row["seconds"])
    print(f"Fastest measured: {fastest['threads']} threads; speedup {fastest['speedup']:.2f}x")
    physical = next((row for row in rows if row["threads"] == cores), None)
    if physical:
        print(f"Speedup at {cores} physical cores: {physical['speedup']:.2f}x")
    slower = [row["threads"] for row in rows if row["threads"] > 1 and row["seconds"] > baseline]
    print(f"Slower than one thread: {slower or 'not observed'}")
    print(f"Measurements: {arguments.output.resolve()}")


if __name__ == "__main__":
    main()
