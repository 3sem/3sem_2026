import matplotlib.pyplot as plt
import numpy as np

tools = ['SHM', 'MSG Queue', 'FIFO']
colors = ['#de425b', '#f8b862', '#488f31'] 

times_4k = [8.6075, 4.1786, 4.0816]

times_64k = [1.5211, 3.6983, 0.9852]
times_1m = [1.0354, 3.7295, 1.1996]

fig, axes = plt.subplots(1, 3, figsize=(15, 5), sharey=False)
fig.suptitle('Сравнение времени передачи файла (~100 МБ) через различные IPC', fontsize=16, fontweight='bold')

bars1 = axes[0].bar(tools, times_4k, color=colors, edgecolor='black', width=0.6)
axes[0].set_title('Буфер: 4096 байт (4 КБ)', fontsize=12)
axes[0].set_ylabel('Время выполнения (секунды)', fontsize=11)
axes[0].grid(axis='y', linestyle='--', alpha=0.7)
axes[0].bar_label(bars1, fmt='%.4f c.', padding=3)

bars2 = axes[1].bar(tools, times_64k, color=colors, edgecolor='black', width=0.6)
axes[1].set_title('Буфер: 65536 байт (64 КБ)', fontsize=12)
axes[1].grid(axis='y', linestyle='--', alpha=0.7)
axes[1].bar_label(bars2, fmt='%.4f c.', padding=3)

bars3 = axes[2].bar(tools, times_1m, color=colors, edgecolor='black', width=0.6)
axes[2].set_title('Буфер: 1048576 байт (1 МБ)', fontsize=12)
axes[2].grid(axis='y', linestyle='--', alpha=0.7)
axes[2].bar_label(bars3, fmt='%.4f c.', padding=3)

# Оптимизируем расположение элементов на холсте и отображаем
plt.tight_layout()
plt.show()
