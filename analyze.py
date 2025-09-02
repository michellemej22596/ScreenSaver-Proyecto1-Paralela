# analyze.py
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os

# lee CSV
df = pd.read_csv('results_raw.csv')

# agrupa y calcula media y std
grouped = df.groupby(['binary','n_threads','N']).agg(
    mean_time_total=('time_total', 'mean'),
    std_time_total=('time_total', 'std'),
    mean_time_update=('time_update', 'mean'),
    std_time_update=('time_update', 'std'),
    reps=('time_total','count')
).reset_index()

# extraer Ts (secuencial) por N
seq = grouped[(grouped['binary']=='screensaver_seq') & (grouped['n_threads']==1)][['N','mean_time_total']].rename(columns={'mean_time_total':'Ts'})
# unir Ts a todas las filas por N
merged = pd.merge(grouped, seq, on='N', how='left')

# calcular speedup y eficiencia
merged['speedup'] = merged['Ts'] / merged['mean_time_total']
merged['efficiency'] = merged['speedup'] / merged['n_threads']

# guardar resumen
merged.to_csv('metrics_summary.csv', index=False)
print("Saved metrics_summary.csv")

# Graficos por N
os.makedirs('plots', exist_ok=True)
unique_N = sorted(merged['N'].unique())

for N in unique_N:
    sub = merged[merged['N']==N].sort_values('n_threads')
    plt.figure()
    plt.errorbar(sub['n_threads'], sub['speedup'], yerr=sub['std_time_total'], marker='o', linestyle='-')
    plt.xticks(sub['n_threads'])
    plt.xlabel('Número de hilos')
    plt.ylabel('Speedup (Ts/Tp)')
    plt.title(f'Speedup vs threads (N={N})')
    plt.grid(True)
    plt.savefig(f'plots/speedup_N{N}.png', bbox_inches='tight')
    plt.close()

    plt.figure()
    plt.plot(sub['n_threads'], sub['efficiency'], marker='o', linestyle='-')
    plt.xticks(sub['n_threads'])
    plt.xlabel('Número de hilos')
    plt.ylabel('Eficiencia (speedup / threads)')
    plt.title(f'Eficiencia vs threads (N={N})')
    plt.grid(True)
    plt.savefig(f'plots/efficiency_N{N}.png', bbox_inches='tight')
    plt.close()

print("Plots saved in ./plots/")
