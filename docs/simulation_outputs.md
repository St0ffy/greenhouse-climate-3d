# Simulation Outputs

Day 6 adds the full simulation loop and output files.

## Simulation Loop

The main simulation function is:

```text
runSimulation(...)
```

It:

- creates the initial 3D cell state;
- stores the initial frame at `t = 0`;
- advances time using `time_step_seconds`;
- calls `advanceClimate(...)` for each step;
- stores every frame;
- calculates plant temperature and humidity metrics;
- accumulates heater energy in kWh.

The final result type is `SimulationResult`.

## Output Directory

Output settings are read from config:

```json
"output": {
  "directory": "outputs",
  "write_csv": true,
  "write_json": true,
  "write_report": true
}
```

## cell_history.csv

This file stores the full 3D grid history.

Columns:

```text
time_seconds,linear_index,x,y,z,temperature_c,humidity_percent
```

Use this file when you want to build 3D slices, heat maps, or animations outside the program.

## metrics.csv

This file stores one row per frame.

It is intended for charts:

- average/min/max temperature;
- average/min/max humidity;
- plant average temperature;
- plant target temperature;
- plant temperature error;
- plant average humidity;
- cumulative heater energy;
- vent temperature and humidity deltas.

## summary.json

This file stores a compact machine-readable summary of the final result.

It is useful if another script or tool needs to read the final metrics without scanning CSV files.

## report.txt

This is a human-readable report.

It is printed in the terminal and saved to:

```text
outputs/report.txt
```

The report is meant to be readable in PuTTY without opening external tools.

## Current Limitation

The simulator stores every frame and every cell. For the default grid this is fine:

```text
361 frames x 288 cells
```

If the grid becomes much larger later, the exporter may need an output interval setting so it does not write every single frame.

