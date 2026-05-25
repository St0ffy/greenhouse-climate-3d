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
- `controller` settings for the fallback controller and the lightweight learning layer;
- `output.terminal_view.interactive` for live terminal control.

Plant sensors are placed above plant coordinates by `sensor_height_m` and mapped to the nearest grid cell. The controller uses these sensor readings, not whole-greenhouse averages.

## Controller

When `controller.enabled` is true, the fallback controller adjusts:

- heater power when plant sensors are colder than targets;
- humidifier mode/level when plant sensors are too dry;
- vent opening when plant sensors are too hot or too humid.

When `controller.ml_enabled` is true, a small in-process learning layer explores small changes around the fallback decision. Its reward favors plant comfort and health while penalizing device energy use. No external ML library is required.

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
