# Heater Placement Optimization

Day 7 adds the `optimize` mode.

## Goal

The optimizer tries to place heaters so plant control points are closer to their target temperature.

It is not complex machine learning. It is a simple offline search:

1. Generate possible heater positions.
2. Run a normal simulation for each tested layout.
3. Score the final result.
4. Keep the best layout.

This is easier to explain, test, and finish within the project scope.

## Command

```bash
./build/greenhouse3d optimize configs/default_config.json
```

On Windows PowerShell after direct `g++` build:

```powershell
.\greenhouse3d.exe optimize configs\default_config.json
```

## Config

Optimizer settings live in `configs/default_config.json`:

```json
"optimizer": {
  "enabled": false,
  "candidate_step_m": 1.0,
  "max_candidates": 30,
  "max_layouts": 500,
  "energy_weight": 0.05
}
```

The command-line mode is the main switch. Even if `enabled` is `false`, running `optimize` starts optimization.

Fields:

- `candidate_step_m` controls spacing between candidate positions on the greenhouse floor.
- `max_candidates` limits how many candidate positions are kept.
- `max_layouts` limits how many heater layouts are simulated.
- `energy_weight` adds an energy-use penalty to the quality score.

If optimization is slow, reduce `max_candidates` or `max_layouts`.

## Quality Function

The optimizer minimizes:

```text
quality = average_plant_temperature_error
        + 0.25 * max_plant_temperature_error
        + energy_weight * heater_energy_kwh
```

Lower is better.

The max-error part matters because average temperature can look good while one plant zone is still too cold or too hot.

## Output

The optimizer prints and saves:

- tested layout count;
- baseline quality;
- best quality;
- baseline plant temperature error;
- best plant temperature error;
- recommended heater coordinates;
- full report for the best simulation.

The best simulation is exported to the same files as normal simulation:

```text
outputs/cell_history.csv
outputs/metrics.csv
outputs/summary.json
outputs/report.txt
```

In optimize mode, `report.txt` contains the optimization report.

