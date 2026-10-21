import matplotlib.pyplot as plt

# -------------------------------------------------------------
# ВХОДНЫЕ ДАННЫЕ (из ваших реальных замеров на скриншотах)
# -------------------------------------------------------------
threads_nosync = [1, 2, 4, 8, 16, 32]
time_nosync = [0.742446, 0.381325, 0.215605, 0.318007, 0.154267, 0.142280]

threads_sync = [1, 2, 4, 8, 16]
time_sync = [0.124574, 0.123598, 0.191197, 0.242116, 0.307029]

# Вычисление ускорения (Speedup = T1 / Tn) для режима без синхронизации
speedup_nosync = [time_nosync[0] / t for t in time_nosync]
# Идеальное (линейное) ускорение для сравнения
ideal_speedup = [1, 2, 4, 8, 16, 32]

# -------------------------------------------------------------
# ПОСТРОЕНИЕ ГРАФИКОВ
# -------------------------------------------------------------
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

# --- График 1: Время выполнения ---
ax1.plot(threads_nosync, time_nosync, 'o-', color='green', linewidth=2, label='Без синхронизации (N=10⁸)')
ax1.plot(threads_sync, time_sync, 'o--', color='red', linewidth=2, label='С синхронизацией (N=10⁷)')
ax1.set_title('Зависимость времени выполнения от числа потоков', fontsize=12)
ax1.set_xlabel('Количество потоков (n)', fontsize=10)
ax1.set_ylabel('Время выполнения (секунды)', fontsize=10)
ax1.set_xticks(threads_nosync)
ax1.grid(True, linestyle='--', alpha=0.6)
ax1.legend(fontsize=10)

ax1.annotate('Стагнация / Флуктуации', xy=(8, 0.318), xytext=(12, 0.45),
             arrowprops=dict(facecolor='black', shrink=0.05, width=1, headwidth=6))

ax2.plot(threads_nosync, speedup_nosync, 'o-', color='blue', linewidth=2, label='Реальное ускорение (Без синхн.)')
ax2.plot(threads_nosync, ideal_speedup, '--', color='gray', alpha=0.7, label='Идеальное ускорение (Линейное)')
ax2.set_title('Коэффициент ускорения производительности S(n)', fontsize=12)
ax2.set_xlabel('Количество потоков (n)', fontsize=10)
ax2.set_ylabel('Масштабируемость (во сколько раз быстрее)', fontsize=10)
ax2.set_xticks(threads_nosync)
ax2.set_ylim(0, 10) # Ограничим видимость до 10 для наглядности физических ядер
ax2.grid(True, linestyle='--', alpha=0.6)
ax2.legend(fontsize=10)

ax2.annotate('Пик на физических ядрах (n=4)', xy=(4, speedup_nosync[2]), xytext=(6, 1.5),
             arrowprops=dict(facecolor='darkblue', shrink=0.05, width=1, headwidth=6))

plt.tight_layout()

plt.savefig('performance_charts.png', dpi=300)
print("График успешно построен и сохранен в файл 'performance_charts.png'")
plt.show()
