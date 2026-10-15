import csv
from pathlib import Path
from statistics import mean
import subprocess
from time import perf_counter

import matplotlib.pyplot as plt


MAX_THREADS = 16384
REPEATS = 5
ZOOM_MAX_THREADS = 128


def thread_counts():
    counts = set(range(1, 65))
    counts.update(range(80, 257, 16))
    counts.update(range(288, 1025, 32))
    counts.update(range(1152, 4097, 128))
    counts.update(range(4352, MAX_THREADS + 1, 256))
    counts.add(MAX_THREADS)

    return sorted(n for n in counts if 1 <= n <= MAX_THREADS)


def measure(program, threads):
    start = perf_counter()

    result = subprocess.run(
        [str(program), str(threads)],
        cwd=program.parent,
        capture_output=True,
        text=True,
    )

    seconds = perf_counter() - start

    if result.returncode != 0:
        message = result.stderr.strip() or result.stdout.strip()
        raise RuntimeError(
            f"n = {threads}, код {result.returncode}:\n{message}"
        )

    return seconds


def draw_plot(ax, counts, times, baseline, title):
    ax.plot(counts, times, "o-", markersize=3, linewidth=1)

    ax.axhline(
        baseline,
        color="gray",
        linestyle="--",
        label="Один поток",
    )

    ax.axvline(
        16,
        color="green",
        linestyle=":",
        label="16 физических ядер",
    )

    ax.set_xlabel("Количество потоков")
    ax.set_ylabel("Среднее время выполнения, с")
    ax.set_title(title)
    ax.set_ylim(bottom=0)
    ax.grid(True, alpha=0.3)
    ax.legend()


def main():
    if MAX_THREADS < 1 or REPEATS < 1 or ZOOM_MAX_THREADS < 1:
        raise SystemExit("Параметры должны быть положительными.")

    directory = Path(__file__).resolve().parent
    program = directory / "integrator"

    if not program.is_file():
        raise SystemExit("Не найден integrator. Сначала собери программу.")

    counts = []
    averages = []

    with (directory / "timings.csv").open(
        "w", newline="", encoding="utf-8"
    ) as file:
        writer = csv.writer(file)
        writer.writerow(["threads", "run", "seconds"])

        for threads in thread_counts():
            times = []

            try:
                for run in range(1, REPEATS + 1):
                    seconds = measure(program, threads)
                    times.append(seconds)

                    writer.writerow([threads, run, f"{seconds:.9f}"])
                    file.flush()

            except RuntimeError as error:
                print(f"Измерения остановлены: {error}", flush=True)
                print("График будет построен по завершённым сериям.")
                break

            counts.append(threads)
            averages.append(mean(times))

            print(
                f"n = {threads:5d}: {averages[-1]:.6f} с, "
                f"ускорение = {averages[0] / averages[-1]:.2f}",
                flush=True,
            )

    if not counts:
        raise SystemExit("Нет завершённых серий для построения графика.")

    fig, axes = plt.subplots(1, 2, figsize=(15, 5))

    draw_plot(
        axes[0],
        counts,
        averages,
        averages[0],
        "Весь диапазон",
    )

    zoom = [
        (n, t)
        for n, t in zip(counts, averages)
        if n <= ZOOM_MAX_THREADS
    ]

    draw_plot(
        axes[1],
        [n for n, _ in zoom],
        [t for _, t in zoom],
        averages[0],
        f"До {ZOOM_MAX_THREADS} потоков",
    )

    fig.suptitle("Время интегрирования в зависимости от числа потоков")
    fig.tight_layout()
    fig.savefig(directory / "integration_threads.png", dpi=180)
    plt.close(fig)

    best = min(range(len(averages)), key=averages.__getitem__)

    print(f"\nЛучшее измеренное среднее: {averages[best]:.6f} с")
    print(f"Число потоков: {counts[best]}")
    print(f"Ускорение: {averages[0] / averages[best]:.2f}")

    slower = next(
        (
            n for n, seconds in zip(counts, averages)
            if n > 1 and seconds > averages[0]
        ),
        None,
    )

    if slower is not None:
        print(
            "Первое проверенное число потоков, при котором "
            f"программа медленнее однопоточной: {slower}"
        )
    else:
        print("Во всех проверенных вариантах время не превышало однопоточное.")

    print("Сохранены timings.csv и integration_threads.png")


if __name__ == "__main__":
    try:
        main()
    except OSError as error:
        raise SystemExit(f"Ошибка: {error}")
