# Control Comparison Mode

The project has three command-line modes:

- `simulate` runs one configured greenhouse scenario.
- `optimize` tests heater positions and reports the best layout.
- `compare` runs the same scenario twice so ordinary sensor automation can be
  compared with the ML-assisted controller.

## What Compare Runs

`compare` forces two non-interactive simulations from one config:

- `on_off`: `controller.enabled=true`, `strategy=on_off`, `ml_enabled=false`.
- `ml`: `controller.enabled=true`, `strategy=proportional`, `ml_enabled=true`,
  `ml_memory_enabled=false`.

The `on_off` strategy is relay automation. It switches heaters, humidifiers,
and vents fully on when plant sensors are outside the target tolerance, then
turns them off when all plant sensors are close enough to the targets.

Default tolerances:

```json
"controller": {
  "temperature_tolerance_c": 0.5,
  "humidity_tolerance_percent": 3.0
}
```

## Output

For a base output directory such as `outputs/examples/control_comparison`, the
mode writes:

```text
outputs/examples/control_comparison/on_off/
outputs/examples/control_comparison/ml/
outputs/examples/control_comparison/comparison_report.txt
```

Each subdirectory contains the usual `metrics.csv`, `summary.json`, and
`report.txt` files according to the config output flags.

The comparison report includes:

- time when all plant sensors first reached the target tolerances;
- final average and max plant temperature error;
- final average and max plant humidity error;
- final plant comfort, health, and alive count;
- heater and total device energy;
- a combined recommendation.

The terminal also prints a short table immediately after the command finishes.
Rows marked `BEST` show which mode is better for each metric, and the `Verdict`
section gives the quick recommendation without opening any output file.

## Main Examples

The main latest large scenario is:

```bash
./build/greenhouse3d simulate configs/default_config.json
```

It is the weekly live-control demo with plant sensors, ML memory, and learning
analysis.

The short comparison scenario is:

```bash
./build/greenhouse3d compare examples/control_comparison/config.json
```

Use it first when comparing ML control with ordinary on/off automation.
