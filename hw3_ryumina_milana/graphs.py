#!/usr/bin/env python3
"""
Строит сразу 6 картинок:

1. Три картинки «по буферу» — для каждого размера файла.
   По X — размеры буфера, фиксирован размер файла.

2. Три картинки «по файлу» — для каждого размера буфера.
   По X — размеры файла, фиксирован размер буфера.

Каждая картинка — 3 гистограммы рядом: FIFO, Shared memory, Message queue.

Использование:
    python3 generate.py results.csv

CSV должен содержать колонки:
    method,file_size,buf_size,time,status
"""

import sys
import csv
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rcParams

# ----------------------------
# Настройки оформления
# ----------------------------
plt.style.use('dark_background')
rcParams['font.size'] = 12
rcParams['font.family'] = 'DejaVu Sans'

COLORS = {
    'FIFO':           '#4A90D9',   # синий
    'Shared memory':  '#2ECC71',   # зелёный
    'Message queue':  '#E67E22',   # оранжевый
}

METHODS = ['FIFO', 'Shared memory', 'Message queue']

# Порядок и метки размеров файлов (для оси X и заголовков)
FILE_SIZES = ['8KB', '4MB', '2GB']
FILE_LABELS = {
    '8KB': '8 КБ',
    '4MB': '4 МБ',
    '2GB': '2 ГБ',
}

# Размеры буфера (числа в байтах) и их метки
BUF_SIZES = [8192, 65536, 1048576]
BUF_LABELS = {
    8192:    '8 КБ',
    65536:   '64 КБ',
    1048576: '1 МБ',
}


# ----------------------------
# Чтение CSV
# ----------------------------
def read_results(csv_path):
    """
    Читает CSV: method,file_size,buf_size,time,status
    Возвращает список dict'ов.
    """
    rows = []
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append({
                'method':    row['method'].strip(),
                'file_size': row['file_size'].strip(),
                'buf_size':  int(row['buf_size']),
                'time':      float(row['time']),
            })
    return rows


# ----------------------------
# Агрегация данных
# ----------------------------
def aggregate_by_buf(rows, file_size):
    """
    Для фиксированного размера файла:
    {method: {buf_size: [times...]}}
    """
    result = {m: {} for m in METHODS}
    for r in rows:
        if r['file_size'] != file_size:
            continue
        method = r['method']
        if method not in result:
            continue
        result[method].setdefault(r['buf_size'], []).append(r['time'])
    return result


def aggregate_by_file(rows, buf_size):
    """
    Для фиксированного размера буфера:
    {method: {file_size: [times...]}}
    """
    result = {m: {} for m in METHODS}
    for r in rows:
        if r['buf_size'] != buf_size:
            continue
        method = r['method']
        if method not in result:
            continue
        result[method].setdefault(r['file_size'], []).append(r['time'])
    return result


# ----------------------------
# Построение одной картинки
# ----------------------------
def plot_figure(title, xlabel, footer, data, x_keys, x_labels,
                output_path):
    """
    data: {method: {x_key: [times...]}}
    x_keys: порядок по X (список ключей)
    x_labels: подписи под X (список строк, той же длины)
    """
    fig, axes = plt.subplots(1, 3, figsize=(16, 6), sharey=True)
    fig.suptitle(title, fontsize=18, fontweight='bold', y=1.02)

    for ax, method in zip(axes, METHODS):
        method_data = data.get(method, {})
        if not method_data:
            ax.set_title(method, fontsize=14, fontweight='bold')
            ax.text(0.5, 0.5, 'нет данных',
                    ha='center', va='center', transform=ax.transAxes)
            continue

        medians, mins, maxs, ns = [], [], [], []
        for k in x_keys:
            times = method_data.get(k, [])
            if not times:
                medians.append(0.0)
                mins.append(0.0)
                maxs.append(0.0)
                ns.append(0)
                continue
            medians.append(float(np.median(times)))
            mins.append(min(times))
            maxs.append(max(times))
            ns.append(len(times))

        lower_err = [m - mn for m, mn in zip(medians, mins)]
        upper_err = [mx - m for mx, m in zip(maxs, medians)]
        yerr = [lower_err, upper_err]

        x = np.arange(len(x_keys))
        color = COLORS.get(method, '#888888')

        ax.bar(x, medians, color=color, width=0.6,
               edgecolor='white', linewidth=1, alpha=0.85,
               yerr=yerr, capsize=5,
               error_kw={'ecolor': 'white', 'linewidth': 1})

        top = max(maxs) if maxs and max(maxs) > 0 else 1.0
        for xi, (med, n) in enumerate(zip(medians, ns)):
            if n == 0:
                continue
            ax.text(xi, med + top * 0.03,
                    f'{med:.3f}\nn={n}',
                    ha='center', va='bottom', fontsize=10)

        ax.set_title(method, fontsize=14, fontweight='bold', pad=12)
        ax.set_xlabel(xlabel, fontsize=12)
        ax.set_xticks(x)
        ax.set_xticklabels(x_labels, fontsize=11)
        ax.grid(True, axis='y', alpha=0.3)
        ax.set_axisbelow(True)

    axes[0].set_ylabel('Время, с', fontsize=12)
    axes[0].set_ylim(bottom=0)

    fig.text(0.5, -0.02,
             'Столбцы — медиана; интервалы — минимум–максимум; '
             'n — число повторений.\n' + footer,
             ha='center', va='top', fontsize=10, style='italic')

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight',
                facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f'  → {output_path}')


# ----------------------------
# Основная логика
# ----------------------------
def main():
    if len(sys.argv) < 2:
        print("Usage: python3 generate.py results.csv")
        sys.exit(1)

    csv_path = sys.argv[1]
    rows = read_results(csv_path)

    print("Строю картинки «по буферу» (фиксирован размер файла):")
    for file_size in FILE_SIZES:
        data = aggregate_by_buf(rows, file_size)
        title = f'Время передачи файла · {FILE_LABELS[file_size]}'
        xlabel = 'Размер порции'
        footer = (f'Файл: {FILE_LABELS[file_size]}. '
                  f'Время включает запуск процессов, '
                  f'синхронизацию и файловый ввод-вывод.')
        x_keys = BUF_SIZES
        x_labels = [BUF_LABELS[b] for b in BUF_SIZES]
        output = f'ipc_by_buf_{file_size}.png'
        plot_figure(title, xlabel, footer, data, x_keys, x_labels, output)

    print("Строю картинки «по файлу» (фиксирован размер буфера):")
    for buf_size in BUF_SIZES:
        data = aggregate_by_file(rows, buf_size)
        title = f'Время передачи файла · буфер {BUF_LABELS[buf_size]}'
        xlabel = 'Размер файла'
        footer = (f'Буфер: {BUF_LABELS[buf_size]}. '
                  f'Время включает запуск процессов, '
                  f'синхронизацию и файловый ввод-вывод.')
        x_keys = FILE_SIZES
        x_labels = [FILE_LABELS[f] for f in FILE_SIZES]
        output = f'ipc_by_file_{buf_size}.png'
        plot_figure(title, xlabel, footer, data, x_keys, x_labels, output)

    print("Готово. Построено 6 картинок.")


if __name__ == '__main__':
    main()