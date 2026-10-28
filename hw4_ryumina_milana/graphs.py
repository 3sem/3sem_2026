#!/usr/bin/env python3
import csv
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import rcParams

plt.style.use('dark_background')
rcParams['font.size'] = 14

# Собираем все значения
data = {'INDEPENDENT': {}, 'SHARED': {}}

with open('results.csv') as f:
    reader = csv.DictReader(f)
    for row in reader:
        mode = row['mode']
        n = int(row['threads'])
        t = float(row['time'])
        if t <= 0:
            continue
        data[mode].setdefault(n, []).append(t)

fig, ax = plt.subplots(figsize=(12, 6))

for mode, color, marker in [('INDEPENDENT', '#4ECDC4', 'o'),
                             ('SHARED', "#1A7F5F", 's')]:
    ns = sorted(data[mode].keys())
    medians = [np.median(data[mode][n]) for n in ns]
    mins = [min(data[mode][n]) for n in ns]
    maxs = [max(data[mode][n]) for n in ns]

    lower = [m - mn for m, mn in zip(medians, mins)]
    upper = [mx - m for mx, m in zip(maxs, medians)]

    ax.errorbar(ns, medians, yerr=[lower, upper],
                fmt=marker + '-', label=mode, color=color,
                linewidth=2, markersize=8, capsize=5)

ax.axvline(x=4, color='yellow', linestyle='--', alpha=0.7, label='4 ядра')
ax.axvline(x=8, color='orange', linestyle='--', alpha=0.7, label='8 логических')

ax.set_xlabel('Число потоков', fontsize=14, fontweight='bold')
ax.set_ylabel('Время, с', fontsize=14, fontweight='bold')
ax.set_title('Монте-Карло: медиана ± мин/макс (n=5)',
             fontsize=16, fontweight='bold')
ax.legend()
ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('graph.png', dpi=150, bbox_inches='tight')
print("График сохранён в graph.png")