Scaled unified plot

The script `generate_plots.py` now includes a "scaled unified plot" that rescales each numeric metric so its maximum equals the global maximum across selected metrics. This makes very small series (e.g., Map MB, System Compression) visible alongside large series (e.g., Byte Count, Compression Ratio).

To regenerate plots:

```bash
.venv/bin/python generate_plots.py benchmark.log
```

Output files:
- `outputs/benchmark_scaled.png` — unified scaled plot (all series scaled to same peak)
- `outputs/benchmark_plot.png` — detailed combined plot (unchanged)

Notes:
- Each legend entry shows the scaling factor applied.
- If you prefer a log-scale or secondary axes instead, tell me and I will add it.
