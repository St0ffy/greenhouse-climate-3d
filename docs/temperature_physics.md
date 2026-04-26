# Temperature Physics

Day 4 implements the first real physics layer.

The goal is not full CFD. The model is deliberately approximate, but structured enough that the later simulator can call it repeatedly.

## Main Function

The main function is:

```text
advanceTemperature(...)
```

It receives:

- current vector of `CellState`;
- `Grid3D`;
- `WeatherCondition` for the current time;
- `MaterialProperties`;
- `MappedDeviceSet`;
- time step in seconds;
- optional temperature physics coefficients.

It returns:

- updated vector of cells;
- `TemperatureStepSummary`.

## Effects Included

### Neighbor Transfer

Each cell moves slightly toward the average temperature of its 6 direct neighbors.

This spreads local heat without implementing fluid flow.

### Boundary Heat Loss

Cells touching the greenhouse boundary move toward the outside temperature.

The material heat loss coefficient controls how strong this effect is.

### Solar Heating

Solar radiation increases temperature.

Upper cells receive a larger share because sunlight enters mostly from the roof and upper sides in this simplified model.

### Heaters

Each heater affects cells inside its radius.

The heat is distributed by distance-based weights that were prepared by `Devices`. This means Physics does not search coordinates again during every step.

## Effects Not Yet Included

Ventilation and humidity are intentionally deferred to the next step.

For now:

- humidity values are copied unchanged;
- vents are already mapped, but not yet used by `advanceTemperature`;
- humidifiers are already mapped, but not yet used by humidity formulas.

## Why This Design Is Useful Later

The future `Simulator` can hold a vector of cells and repeatedly call:

```text
cells = advanceTemperature(cells, ...).cells
```

Then day 5 can extend this same pattern with ventilation and humidity instead of replacing the whole design.

