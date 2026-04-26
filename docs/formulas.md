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

The current implementation uses only 6 direct neighbors, not diagonals. This keeps the model simple and stable enough for an educational one-week project.

## Heat Loss Through Greenhouse Cover

Boundary cells lose heat to the outside air.

```text
heat_loss = material_loss_coefficient * (inside_temperature - outside_temperature) * dt
```

The larger the difference between inside and outside temperature, the stronger the loss.

In code the sign is written as a movement toward outside temperature:

```text
boundary_delta = material_loss * boundary_rate * (outside_temperature - inside_temperature) * dt
```

So if outside air is colder, the delta is negative.

## Solar Heating

Solar radiation increases temperature, mostly near the upper cells.

```text
solar_gain = solar_radiation * material_solar_transmission * solar_gain_coefficient * dt
```

The current implementation also multiplies this by a height factor, so upper cells receive more solar heat than lower cells.

## Ventilation

Open vents move nearby cells toward outside temperature and humidity.

```text
new_value = old_value + opening * ventilation_rate * (outside_value - old_value) * dt
```

The current implementation applies this formula to both temperature and humidity for cells inside each vent influence radius. The mixing factor is clamped, so a large time step cannot instantly replace the entire cell state.

## Heaters

Heaters add heat to cells near their position. The influence should become weaker with distance.

```text
heater_gain = heater_power * distance_factor * heater_coefficient * dt
```

The current implementation distributes each heater's total gain between influenced cells by distance-based weights. This prevents a larger influence radius from accidentally creating much more total heat.

## Temperature Step Summary

Each temperature step returns:

- min, max, and average temperature;
- average absolute neighbor exchange;
- average boundary temperature delta;
- average solar gain;
- average heater gain;
- heater energy for the step in kWh.

## Humidifiers

Humidifiers increase humidity near their position. The amount depends on mode: `off`, `low`, `medium`, or `high`.

The current implementation distributes each humidifier's total moisture gain between influenced cells by distance-based weights. Humidity is clamped to `0..100%`.

## Humidity Transfer

Humidity also spreads between neighboring cells:

```text
new_humidity = old_humidity + humidity_k * (neighbor_average_humidity - old_humidity) * dt
```

This mirrors the simplified heat-transfer idea, but uses separate coefficients.

## Optimization Quality

A simple quality function can combine plant temperature error and energy use.

```text
quality = average_abs(plant_temperature - target_temperature) + energy_weight * energy_kwh
```

Lower quality is better.
