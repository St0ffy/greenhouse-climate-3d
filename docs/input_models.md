# Input Models

Day 3 prepares the data objects that the physics module will use.

## Config

`Config` reads `configs/default_config.json` and produces `SimulationConfig`.

The parser is intentionally small and dependency-free. It is good enough for this fixed educational config structure. If the project later needs more flexible JSON support, this is the main place to rewrite with a real JSON library.

Current output groups:

- greenhouse size;
- grid size;
- run duration and time step;
- initial temperature and humidity;
- weather specification;
- material specification;
- vents, heaters, humidifiers, and plant control points.

## Weather

`WeatherTimeline` supports two modes:

- constant weather from config;
- CSV weather from `weather_file`.

CSV format:

```text
time_seconds,outside_temperature_c,outside_humidity_percent,solar_radiation_w_m2
0,4.0,82.0,0.0
3600,5.0,80.0,120.0
```

The timeline sorts samples by time and interpolates between them.

Example: if `t = 1800` is between samples at `0` and `3600`, the module returns halfway values.

## Material

`Material` converts config values into `MaterialProperties`.

Supported presets:

```text
polycarbonate: heat loss 0.35, solar transmission 0.75
glass:         heat loss 0.55, solar transmission 0.90
film:          heat loss 0.70, solar transmission 0.80
```

The config may override coefficients. Solar transmission must stay in `0..1`.

## Devices

`Devices` now has two layers:

- raw specs from config: `VentSpec`, `HeaterSpec`, `HumidifierSpec`, `PlantPoint`;
- mapped specs for simulation: `MappedVent`, `MappedHeater`, `MappedHumidifier`, `MappedPlantPoint`.

The combined object is `MappedDeviceSet`.

It contains:

- mapped plant control cells;
- mapped vent influence cells;
- mapped heater influence cells;
- mapped humidifier influence cells;
- total heater power;
- average vent opening.

## Why This Matters For Physics

Physics should not read config text, parse CSV files, or search for device cells repeatedly.

By the time Physics starts, it should already have:

- `Grid3D`;
- `WeatherTimeline`;
- `MaterialProperties`;
- `MappedDeviceSet`;
- current vector of cell states.

This keeps the future temperature and humidity formulas focused and easier to test.

