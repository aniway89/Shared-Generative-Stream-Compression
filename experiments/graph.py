#!/usr/bin/env python3
"""
Compression Analysis Graph Generator
Creates visualizations from compression performance data
"""

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.patches import Patch
import warnings
warnings.filterwarnings('ignore')

# Set style for better looking graphs
plt.style.use('seaborn-v0_8-darkgrid')
plt.rcParams['figure.figsize'] = (12, 8)
plt.rcParams['font.size'] = 11

# Data from analysis
data = {
    'Image': ['PTI2 cp.png', 'PTI2 cp.png', 'PTI2.png', 'PTI2.png', 'PTI2.png', 
              'PTI1 cp.jpg', 'PTI1 cp.jpg', 'PTI1.jpg', 'Big mars map 2.png'],
    'Original_MB': [0.755476, 0.755476, 3.437353, 3.437353, 3.437353, 
                    0.0152, 0.0152, 0.2415, 82.95],
    'Compressed_MB': [0.000004, 0.000004, 1.414494, 0.000023, 0.000023, 
                      0.00509, 0.00509, 0.000027, 0.000027],
    'Compression_Ratio': [198043.5, 198043.5, 2.43, 150180.25, 150180.25, 
                          2.985, 2.985, 9044.18, 3106258.11],
    'Matching_Time': [None, None, None, 0.068231, 0.067171, 
                      527.960368, 532.547549, 0.019252, 2.18694],
    'Stream_Time': [None, None, None, 0.103215, 0.091675, 
                    0.142253, 0.022925, 0.025467, 4.169645],
    'Recon_Time': [None, None, None, 0.18004, 0.161694, 
                   None, None, None, None]
}

# Create DataFrame
df = pd.DataFrame(data)

# Filter for valid compression data
df_comp = df[df['Compression_Ratio'].notna()].copy()

# Filter for timing data
df_timing = df[df['Matching_Time'].notna()].copy()

# Create a figure with subplots
fig = plt.figure(figsize=(16, 12))
fig.suptitle('Compression Analysis Dashboard', fontsize=16, fontweight='bold', y=0.98)

# 1. Compression Ratio Comparison (Bar Chart - Log Scale)
ax1 = plt.subplot(2, 3, 1)
images = df_comp['Image'].unique()
ratios = []
for img in images:
    ratio = df_comp[df_comp['Image'] == img]['Compression_Ratio'].iloc[0]
    ratios.append(ratio)

colors = plt.cm.viridis(np.linspace(0, 1, len(images)))
bars = ax1.bar(range(len(images)), ratios, color=colors)
ax1.set_yscale('log')
ax1.set_xticks(range(len(images)))
ax1.set_xticklabels(images, rotation=45, ha='right', fontsize=8)
ax1.set_ylabel('Compression Ratio (log scale)', fontsize=11)
ax1.set_title('Compression Ratio by Image', fontsize=12, fontweight='bold')
ax1.grid(True, alpha=0.3, axis='y')

# Add value labels on bars
for bar, ratio in zip(bars, ratios):
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2., height,
             f'{ratio:,.0f}', ha='center', va='bottom', fontsize=8, rotation=0)

# 2. Original vs Compressed Size Comparison
ax2 = plt.subplot(2, 3, 2)
x = np.arange(len(images))
width = 0.35

original_sizes = []
compressed_sizes = []
for img in images:
    original_sizes.append(df_comp[df_comp['Image'] == img]['Original_MB'].iloc[0])
    compressed_sizes.append(df_comp[df_comp['Image'] == img]['Compressed_MB'].iloc[0])

bars1 = ax2.bar(x - width/2, original_sizes, width, label='Original Size', color='#FF6B6B', alpha=0.8)
bars2 = ax2.bar(x + width/2, compressed_sizes, width, label='Compressed Size', color='#4ECDC4', alpha=0.8)

ax2.set_xticks(x)
ax2.set_xticklabels(images, rotation=45, ha='right', fontsize=8)
ax2.set_ylabel('Size (MB)', fontsize=11)
ax2.set_title('Original vs Compressed Size', fontsize=12, fontweight='bold')
ax2.legend()
ax2.set_yscale('log')
ax2.grid(True, alpha=0.3, axis='y')

# 3. Space Savings (Percentage)
ax3 = plt.subplot(2, 3, 3)
space_savings = []
for i in range(len(images)):
    saving = (1 - compressed_sizes[i] / original_sizes[i]) * 100
    space_savings.append(saving)

colors_savings = ['#2ECC71' if s > 99 else '#E74C3C' for s in space_savings]
bars3 = ax3.bar(range(len(images)), space_savings, color=colors_savings)
ax3.set_xticks(range(len(images)))
ax3.set_xticklabels(images, rotation=45, ha='right', fontsize=8)
ax3.set_ylabel('Space Saved (%)', fontsize=11)
ax3.set_title('Compression Efficiency', fontsize=12, fontweight='bold')
ax3.set_ylim(0, 105)
ax3.axhline(y=99, color='green', linestyle='--', alpha=0.5, label='99% Savings')
ax3.legend()
ax3.grid(True, alpha=0.3, axis='y')

# Add value labels
for bar, saving in zip(bars3, space_savings):
    height = bar.get_height()
    ax3.text(bar.get_x() + bar.get_width()/2., height,
             f'{saving:.2f}%', ha='center', va='bottom', fontsize=9)

# 4. Timing Comparison (for entries with timing data)
ax4 = plt.subplot(2, 3, 4)
timing_images = df_timing['Image'].tolist()
matching_times = df_timing['Matching_Time'].tolist()
stream_times = df_timing['Stream_Time'].tolist()

x_timing = np.arange(len(timing_images))
width = 0.35

bars_matching = ax4.bar(x_timing - width/2, matching_times, width, label='Matching Time', color='#FFA07A', alpha=0.8)
bars_stream = ax4.bar(x_timing + width/2, stream_times, width, label='Stream Generation', color='#87CEEB', alpha=0.8)

ax4.set_xticks(x_timing)
ax4.set_xticklabels(timing_images, rotation=45, ha='right', fontsize=8)
ax4.set_ylabel('Time (seconds)', fontsize=11)
ax4.set_title('Processing Times Comparison', fontsize=12, fontweight='bold')
ax4.legend()
ax4.set_yscale('log')
ax4.grid(True, alpha=0.3, axis='y')

# 5. Scatter Plot: Matching Time vs Stream Generation
ax5 = plt.subplot(2, 3, 5)
df_timing_filtered = df_timing[df_timing['Matching_Time'] < 600]  # Filter extreme outlier
scatter = ax5.scatter(df_timing_filtered['Stream_Time'], df_timing_filtered['Matching_Time'], 
                      s=100, c=range(len(df_timing_filtered)), cmap='plasma', alpha=0.7, edgecolors='black', linewidth=1)

for idx, row in df_timing_filtered.iterrows():
    ax5.annotate(row['Image'], (row['Stream_Time'], row['Matching_Time']), 
                 fontsize=8, alpha=0.7, xytext=(5, 5), textcoords='offset points')

ax5.set_xlabel('Stream Generation Time (sec)', fontsize=11)
ax5.set_ylabel('Matching Time (sec)', fontsize=11)
ax5.set_title('Correlation: Matching vs Stream Time', fontsize=12, fontweight='bold')
ax5.grid(True, alpha=0.3)
cbar = plt.colorbar(scatter, ax=ax5)
cbar.set_label('Data Point Index', fontsize=10)

# Add trend line
if len(df_timing_filtered) > 1:
    z = np.polyfit(df_timing_filtered['Stream_Time'], df_timing_filtered['Matching_Time'], 1)
    p = np.poly1d(z)
    ax5.plot(df_timing_filtered['Stream_Time'].sort_values(), 
             p(df_timing_filtered['Stream_Time'].sort_values()), 
             "r--", alpha=0.5, label=f'Trend: r={z[0]:.2f}')
    ax5.legend()

# 6. Reconstruction Time Analysis - FIXED LINE (changed edgecolors to edgecolor)
ax6 = plt.subplot(2, 3, 6)
recon_data = df[df['Recon_Time'].notna()].copy()
if len(recon_data) > 0:
    recon_images = recon_data['Image'].tolist()[:3]  # Take first 3
    recon_times = recon_data['Recon_Time'].tolist()[:3]
    
    # FIX: Changed 'edgecolors' to 'edgecolor'
    bars_recon = ax6.bar(range(len(recon_images)), recon_times, color='#98D8C8', edgecolor='black', linewidth=1)
    ax6.set_xticks(range(len(recon_images)))
    ax6.set_xticklabels(recon_images, rotation=45, ha='right', fontsize=9)
    ax6.set_ylabel('Time (seconds)', fontsize=11)
    ax6.set_title('Reconstruction Time (Successful Only)', fontsize=12, fontweight='bold')
    ax6.grid(True, alpha=0.3, axis='y')
    
    # Add value labels
    for bar, time in zip(bars_recon, recon_times):
        height = bar.get_height()
        ax6.text(bar.get_x() + bar.get_width()/2., height,
                 f'{time:.3f}s', ha='center', va='bottom', fontsize=10)
else:
    ax6.text(0.5, 0.5, 'No reconstruction data available', 
             ha='center', va='center', transform=ax6.transAxes, fontsize=12)
    ax6.set_title('Reconstruction Time', fontsize=12, fontweight='bold')

plt.tight_layout()
plt.savefig('compression_analysis_dashboard.png', dpi=300, bbox_inches='tight', facecolor='white')
plt.show()
print("✅ Dashboard saved as 'compression_analysis_dashboard.png'")

# Additional detailed plots

# Figure 2: Compression Ratio Distribution
fig2, ax = plt.subplots(figsize=(10, 6))
log_ratios = np.log10(df_comp['Compression_Ratio'].values)
ax.hist(log_ratios, bins=15, color='#3498DB', edgecolor='black', alpha=0.7)
ax.set_xlabel('Log10(Compression Ratio)', fontsize=12)
ax.set_ylabel('Frequency', fontsize=12)
ax.set_title('Distribution of Compression Ratios (Log Scale)', fontsize=14, fontweight='bold')
ax.grid(True, alpha=0.3)

# Add interpretation
mean_log = np.mean(log_ratios)
ax.axvline(mean_log, color='red', linestyle='--', linewidth=2, label=f'Mean: 10^{mean_log:.1f}')
ax.legend()

plt.tight_layout()
plt.savefig('compression_distribution.png', dpi=300, bbox_inches='tight', facecolor='white')
plt.show()
print("✅ Distribution plot saved as 'compression_distribution.png'")

# Figure 3: Performance Heatmap
fig3, ax = plt.subplots(figsize=(10, 8))

# Create performance matrix
perf_data = []
labels = []
for idx, row in df_timing.iterrows():
    if row['Matching_Time'] and row['Stream_Time']:
        # Normalize times
        matching_norm = min(row['Matching_Time'] / 600, 1) if row['Matching_Time'] else 0
        stream_norm = min(row['Stream_Time'] / 5, 1) if row['Stream_Time'] else 0
        perf_data.append([1-matching_norm, 1-stream_norm])  # 1 = better performance
        labels.append(row['Image'])

if perf_data:
    im = ax.imshow(perf_data, cmap='RdYlGn', aspect='auto', vmin=0, vmax=1)
    ax.set_xticks([0, 1])
    ax.set_xticklabels(['Matching Speed', 'Stream Generation Speed'])
    ax.set_yticks(range(len(labels)))
    ax.set_yticklabels(labels, fontsize=9)
    ax.set_title('Performance Heatmap (Green = Better)', fontsize=14, fontweight='bold')
    
    # Add colorbar
    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label('Performance Score', fontsize=10)
    
    plt.tight_layout()
    plt.savefig('performance_heatmap.png', dpi=300, bbox_inches='tight', facecolor='white')
    plt.show()
    print("✅ Performance heatmap saved as 'performance_heatmap.png'")
else:
    print("⚠️ Not enough data for performance heatmap")

# Export data to CSV for further analysis
df_comp_export = df_comp[['Image', 'Original_MB', 'Compressed_MB', 'Compression_Ratio']].copy()
df_comp_export['Space_Saved_Percent'] = (1 - df_comp_export['Compressed_MB'] / df_comp_export['Original_MB']) * 100

df_timing_export = df_timing[['Image', 'Matching_Time', 'Stream_Time', 'Recon_Time']].copy()

df_comp_export.to_csv('compression_data.csv', index=False)
df_timing_export.to_csv('timing_data.csv', index=False)
print("✅ Data exported to 'compression_data.csv' and 'timing_data.csv'")

print("\n" + "="*50)
print("SUMMARY STATISTICS")
print("="*50)
print(f"Average Compression Ratio: {df_comp['Compression_Ratio'].mean():,.2f}")
print(f"Median Compression Ratio: {df_comp['Compression_Ratio'].median():,.2f}")
print(f"Best Compression Ratio: {df_comp['Compression_Ratio'].max():,.2f}")
print(f"Average Space Saving: {(1 - df_comp['Compressed_MB'].sum() / df_comp['Original_MB'].sum()) * 100:.2f}%")
print(f"Average Matching Time: {df_timing['Matching_Time'].mean():.2f} sec")
print(f"Average Stream Generation Time: {df_timing['Stream_Time'].mean():.4f} sec")
if df_timing['Recon_Time'].notna().any():
    print(f"Average Reconstruction Time: {df_timing['Recon_Time'].dropna().mean():.4f} sec")git 