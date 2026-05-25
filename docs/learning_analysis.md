# ML learning analysis

This project can analyze `outputs/metrics.csv` after a simulation and build plots
that show whether the adaptive controller improves over time.

## Run simulation

```bash
./build/greenhouse3d simulate configs/default_config.json
```

The simulation should write `outputs/metrics.csv`. Make sure CSV output is
enabled in the config:

```json
"output": {
  "write_csv": true
}
```

## Build plots and reports

```bash
python3 tools/analyze_learning.py outputs/metrics.csv
```

Optional output directory:

```bash
python3 tools/analyze_learning.py outputs/metrics.csv --out outputs/plots
```

Optional comparison window size:

```bash
python3 tools/analyze_learning.py outputs/metrics.csv --window-hours 6
```

If Python dependencies are missing, install them:

```bash
pip install pandas matplotlib
```

## Output files

The analyzer creates `outputs/plots` automatically and writes:

- `ml_reward_over_time.png`
- `plant_comfort_over_time.png`
- `plant_health_over_time.png`
- `energy_over_time.png`
- `climate_errors_over_time.png`
- `ml_learning_summary.png`
- `day_comparison.csv`
- `day_comparison.png`
- `window_comparison.csv`
- `window_comparison.png`
- `learning_report.txt`
- `learning_summary.json`

Some plots are skipped when the matching CSV columns are absent. The script
prints warnings instead of failing.

## How to read the result

Open `learning_report.txt` first. It compares the first third and the last
third of the run and gives a cautious conclusion.

Useful signs that ML is learning:

- `controller_reward` rises over time.
- `plant_avg_comfort` rises or stays high.
- `plant_avg_temperature_error_c` falls.
- Energy does not grow much faster than comfort/reward.
- Later rows in `window_comparison.csv` look better than earlier rows.

For short runs under 1-2 days, day-by-day comparison is weak. Use
`window_comparison.csv` and `window_comparison.png`, because they split the
run into smaller time windows.
