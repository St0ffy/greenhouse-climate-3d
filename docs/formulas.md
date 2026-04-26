# Simplified Formulas

This project intentionally uses approximate formulas. It is not a CFD solver.

## Heat Transfer Between Neighbor Cells

Each cell exchanges heat with its direct neighbors.

```text
new_temperature = old_temperature + k * (neighbor_average - old_temperature) * dt
```

Where:

- `k` is a small heat transfer coefficient.
- `dt` is the time step in seconds.
- `neighbor_average` is the average temperature of adjacent cells.

## Heat Loss Through Greenhouse Cover

Boundary cells lose heat to the outside air.

```text
heat_loss = material_loss_coefficient * (inside_temperature - outside_temperature) * dt
```

The larger the difference between inside and outside temperature, the stronger the loss.

## Solar Heating

Solar radiation increases temperature, mostly near the upper cells.

```text
solar_gain = solar_radiation * material_solar_transmission * solar_gain_coefficient * dt
```

## Ventilation

Open vents move nearby cells toward outside temperature and humidity.

```text
new_value = old_value + opening * ventilation_rate * (outside_value - old_value) * dt
```

## Heaters

Heaters add heat to cells near their position. The influence should become weaker with distance.

```text
heater_gain = heater_power * distance_factor * heater_coefficient * dt
```

## Humidifiers

Humidifiers increase humidity near their position. The amount depends on mode: `off`, `low`, `medium`, or `high`.

## Optimization Quality

A simple quality function can combine plant temperature error and energy use.

```text
quality = average_abs(plant_temperature - target_temperature) + energy_weight * energy_kwh
```

Lower quality is better.

