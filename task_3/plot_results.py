#!/usr/bin/env python3
"""Строит сводку и три столбчатые диаграммы по CSV прогона."""

import csv
import sys
from collections import defaultdict
from pathlib import Path
from xml.sax.saxutils import escape


METHODS = ("shm", "mq", "fifo")
TITLES = {
    "shm": "Разделяемая память SysV",
    "mq": "Очередь сообщений SysV",
    "fifo": "Именованный канал (FIFO)",
}
BUF_LABELS = {
    8192: "8 КиБ",
    65536: "64 КиБ",
    1048576: "1 МиБ",
}
BAR_COLORS = ("#8ecae6", "#219ebc", "#023047")


def median(values):
    xs = sorted(values)
    n = len(xs)
    mid = n // 2
    if n % 2 == 1:
        return xs[mid]
    return 0.5 * (xs[mid - 1] + xs[mid])


def load(path):
    groups = defaultdict(list)
    nbytes = None
    with open(path, newline="") as fh:
        for row in csv.DictReader(fh):
            key = (row["method"], int(row["bufsize"]))
            groups[key].append(float(row["seconds"]))
            nbytes = int(row["bytes"])
    if nbytes is None:
        raise SystemExit("пустой CSV замеров")
    return groups, nbytes


def summarize(groups):
    rows = []
    for method in METHODS:
        for buf in (8192, 65536, 1048576):
            xs = groups.get((method, buf))
            if not xs:
                continue
            med = median(xs)
            rows.append(
                {
                    "method": method,
                    "bufsize": buf,
                    "n": len(xs),
                    "min": min(xs),
                    "median": med,
                    "max": max(xs),
                    "mib_s": 0.0 if med == 0 else (xs and None),
                }
            )
    return rows


def write_summary(rows, nbytes, path):
    with open(path, "w", newline="") as fh:
        writer = csv.DictWriter(
            fh,
            fieldnames=("method", "bufsize", "n", "min", "median", "max", "mib_s"),
        )
        writer.writeheader()
        for row in rows:
            med = row["median"]
            mib_s = 0.0 if med == 0 else (nbytes / (1024 * 1024)) / med
            writer.writerow(
                {
                    "method": row["method"],
                    "bufsize": row["bufsize"],
                    "n": row["n"],
                    "min": f"{row['min']:.9f}",
                    "median": f"{med:.9f}",
                    "max": f"{row['max']:.9f}",
                    "mib_s": f"{mib_s:.3f}",
                }
            )


def nice_max(value):
    if value <= 0:
        return 1.0
    return value * 1.18


def fmt_sec(value, ymax):
    digits = 4 if ymax < 0.1 else 3
    return f"{value:.{digits}f}"


def write_svg(method, rows, nbytes, path, ymax):
    chosen = [r for r in rows if r["method"] == method]
    chosen.sort(key=lambda r: r["bufsize"])
    if len(chosen) != 3:
        raise SystemExit(f"для {method} нужно 3 размера буфера, есть {len(chosen)}")

    width, height = 760, 480
    left, right, top, bottom = 78, 28, 64, 78
    plot_w = width - left - right
    plot_h = height - top - bottom
    slot = plot_w / 3
    bar_w = slot * 0.52
    mib = nbytes / (1024 * 1024)

    parts = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        f'<text x="{width / 2}" y="32" text-anchor="middle" '
        'font-family="sans-serif" font-size="20" fill="#14213d">'
        f"{escape(TITLES[method])}</text>",
        f'<text x="{width / 2}" y="54" text-anchor="middle" '
        'font-family="sans-serif" font-size="13" fill="#555555">'
        f"медиана времени передачи, файл {mib:.0f} МиБ</text>",
    ]

    for i in range(5):
        frac = i / 4
        y = top + plot_h - frac * plot_h
        label = fmt_sec(ymax * frac, ymax)
        parts.append(
            f'<line x1="{left}" y1="{y:.1f}" x2="{left + plot_w}" y2="{y:.1f}" '
            'stroke="#e5e5e5" stroke-width="1"/>'
        )
        parts.append(
            f'<text x="{left - 10}" y="{y + 4:.1f}" text-anchor="end" '
            'font-family="sans-serif" font-size="12" fill="#333333">'
            f"{label}</text>"
        )

    parts.append(
        f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top + plot_h}" '
        'stroke="#222222" stroke-width="1.2"/>'
    )
    parts.append(
        f'<line x1="{left}" y1="{top + plot_h}" x2="{left + plot_w}" y2="{top + plot_h}" '
        'stroke="#222222" stroke-width="1.2"/>'
    )
    parts.append(
        f'<text x="18" y="{top + plot_h / 2}" text-anchor="middle" '
        f'transform="rotate(-90 18 {top + plot_h / 2})" '
        'font-family="sans-serif" font-size="13" fill="#222222">секунды</text>'
    )

    for i, row in enumerate(chosen):
        value = row["median"]
        h = 0 if ymax == 0 else (value / ymax) * plot_h
        x = left + i * slot + (slot - bar_w) / 2
        y = top + plot_h - h
        color = BAR_COLORS[i]
        label = BUF_LABELS.get(row["bufsize"], str(row["bufsize"]))
        parts.append(
            f'<rect x="{x:.1f}" y="{y:.1f}" width="{bar_w:.1f}" height="{h:.1f}" '
            f'fill="{color}" rx="3"/>'
        )
        parts.append(
            f'<text x="{x + bar_w / 2:.1f}" y="{y - 8:.1f}" text-anchor="middle" '
            'font-family="sans-serif" font-size="13" fill="#14213d">'
            f"{fmt_sec(value, ymax)} с</text>"
        )
        parts.append(
            f'<text x="{x + bar_w / 2:.1f}" y="{top + plot_h + 24}" text-anchor="middle" '
            'font-family="sans-serif" font-size="14" fill="#14213d">'
            f"{escape(label)}</text>"
        )

    parts.append(
        f'<text x="{width / 2}" y="{height - 16}" text-anchor="middle" '
        'font-family="sans-serif" font-size="13" fill="#555555">'
        "размер буфера</text>"
    )
    parts.append("</svg>")
    path.write_text("\n".join(parts) + "\n", encoding="utf-8")


def main():
    if len(sys.argv) != 3:
        raise SystemExit(f"использование: {sys.argv[0]} runs.csv каталог_отчёта")
    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    groups, nbytes = load(src)
    rows = summarize(groups)
    write_summary(rows, nbytes, out / "summary.csv")
    ymax = nice_max(max(row["median"] for row in rows))
    for method in METHODS:
        write_svg(method, rows, nbytes, out / f"hist_{method}.svg", ymax)
    print(f"сводка: {out / 'summary.csv'}")
    for row in rows:
        print(
            f"{row['method']:4} {row['bufsize']:7}  "
            f"median {row['median']:.6f}  "
            f"min {row['min']:.6f}  max {row['max']:.6f}"
        )


if __name__ == "__main__":
    main()
