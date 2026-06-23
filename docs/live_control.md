# Live Plant Control

The simulator now supports a plant-centered live control loop:

```text
read plant sensors -> controller decision -> climate physics -> plant growth -> terminal view/input
```

## Config

All initial data stays in one scenario JSON file:

- greenhouse size and grid;
- plants with target temperature, humidity, light, survival ranges, initial health, growth, and sensor height;
- heaters, humidifiers, and vents with `enabled`, `failed`, runtime power/opening/level, and maximum heater power;
- `plant_physics` settings for small humidity uptake by plants;
- `controller` settings for the fallback controller and the lightweight learning layer;
- `output.terminal_view.interactive` for live terminal control.

Plant sensors are placed above plant coordinates by `sensor_height_m` and mapped to the nearest grid cell. The controller uses these sensor readings, not whole-greenhouse averages.

## Controller

When `controller.enabled` is true, the fallback controller adjusts:

- heater power when plant sensors are colder than targets;
- humidifier mode/level when plant sensors are too dry;
- vent opening when plant sensors are too hot or too humid.

The default `controller.strategy` is `proportional`, which keeps the original
smooth behavior. The `on_off` strategy is relay automation: devices switch
fully on when any plant sensor is outside the configured tolerance and switch
off when all plant sensors are close enough to their targets.

When `controller.ml_enabled` is true, a small in-process learning layer explores small changes around the fallback decision. Its reward favors plant comfort and health while penalizing device energy use. No external ML library is required.

Plants also remove a small amount of humidity from their local cell each step. This makes humidity drift downward over time, so humidifiers visibly switch between active and idle states instead of staying irrelevant.

## Interactive Terminal

In `simulate` mode, if `output.terminal_view.interactive` is true and the program is running in an interactive terminal, the simulation advances step by step while accepting input:

```text
Arrows/WASD  move selected cell
Enter/F      fail or repair a device on the selected cell
T            cycle displayed field: temperature, humidity, light, temperature error
Z/X          change z layer
Space        pause/resume
Esc          finish the live view and export results
```

Failed devices are shown with lowercase symbols: `h`, `m`, `v`. The program keeps running after a failure; the controller reallocates work to the remaining active devices when possible.

Plant sensors are shown separately from devices:

```text
S  sensor needs heat or humidity
s  sensor is stable near the plant target
!  sensor is too hot or too humid and ventilation may help
```

With colors enabled, active sensors are yellow, stable sensors are green, and high sensors are red. Selecting a plant or sensor cell prints the sensor cell, target values, measured temperature, humidity, light, and status.

## Weekly Demo

The default config is a larger weekly demonstration:

```bash
./build/greenhouse3d simulate configs/default_config.json
```

It uses a long `48 x 8 x 4` grid, symmetric heater and humidifier placement, and side vents along the left and right greenhouse edges. CSV output is disabled in this config to avoid writing a very large `cell_history.csv` for the full week; JSON and report output remain enabled.
