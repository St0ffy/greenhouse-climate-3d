# Humidity And Ventilation

Day 5 extends the physics layer from temperature-only to a full simplified climate step.

The main function for future simulation is:

```text
advanceClimate(...)
```

It calls `advanceTemperature(...)` first, then applies humidity and ventilation effects.

## Humidity Diffusion

Humidity moves between neighboring cells in the same style as heat:

```text
humidity_delta = humidity_rate * (neighbor_average_humidity - current_humidity) * dt
```

Only 6 direct neighbors are used. This keeps the model understandable and avoids pretending to be a real airflow solver.

## Humidifiers

Humidifiers add moisture to cells inside their influence radius.

Modes:

```text
off    -> 0.0x
low    -> 0.5x
medium -> 1.0x
high   -> 1.5x
```

The moisture is distributed by the same distance-based influence weights used for heaters. This means increasing the radius spreads the moisture over more cells instead of creating unlimited extra moisture.

## Ventilation

Each open vent moves nearby cells toward outside air.

For temperature:

```text
temperature += mix * (outside_temperature - temperature)
```

For humidity:

```text
humidity += mix * (outside_humidity - humidity)
```

The `mix` value depends on:

- vent opening `0..1`;
- distance-based cell influence weight;
- ventilation rate;
- time step.

The mix is clamped so one step cannot unrealistically replace all air in a cell.

## Humidity Switch

The config option:

```json
"humidity": {
  "enabled": true
}
```

controls whether humidity is updated.

If it is `false`:

- temperature physics still works;
- vent temperature exchange still works;
- humidity is copied unchanged;
- humidifiers are ignored;
- vent humidity exchange is skipped.

## Current Simplifications

This model does not calculate:

- condensation;
- saturation vapor pressure;
- plant transpiration;
- real airflow paths;
- pressure differences.

Those would be beyond the planned one-week educational scope. The current model is enough to show how humidity and ventilation influence a 3D grid over time.

