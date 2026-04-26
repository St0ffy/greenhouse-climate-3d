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

## Related Day 5 Layer

Ventilation and humidity are implemented in `advanceClimate`.

`advanceTemperature` is still useful as a focused temperature-only function and for tests. `advanceClimate` calls it first, then applies:

- humidity diffusion between neighboring cells;
- humidifier moisture gain;
- vent temperature exchange with outside air;
- vent humidity exchange with outside air.

## Why This Design Is Useful Later

The future `Simulator` can hold a vector of cells and repeatedly call:

```text
cells = advanceClimate(cells, ...).cells
```

This keeps the time loop simple while leaving temperature-only tests available.

