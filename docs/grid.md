# 3D Grid Design

## Coordinate System

The greenhouse is represented as a rectangular 3D volume.

```text
x: length direction, from 0 to length_m
y: width direction, from 0 to width_m
z: height direction, from 0 to height_m
```

For the default configuration:

```text
greenhouse size: 12 m x 6 m x 3 m
grid size:       12 x 6 x 4
cell size:       1 m x 1 m x 0.75 m
```

## Cell Index

Every cell has a zero-based 3D index:

```text
(x, y, z)
```

Example:

```text
(0, 0, 0)   first lower corner cell
(11, 5, 3)  last upper corner cell in the default grid
```

## Linear Index

Simulation state will be stored in a one-dimensional vector:

```cpp
std::vector<CellState> cells;
```

The linear index formula is:

```text
linear = x + nx * (y + ny * z)
```

This keeps storage simple and fast while still allowing 3D logic through `GridIndex`.

## Coordinate To Cell

User config uses real coordinates in meters. `Grid3D::cellAt(position)` converts those coordinates to the matching grid cell.

If a coordinate is exactly on the outer greenhouse wall, it belongs to the last cell on that axis. For example, in a 12-meter greenhouse with 12 cells along `x`, `x = 12.0` maps to cell `x = 11`.

Coordinates outside the greenhouse throw an error. This is intentional: a heater or plant outside the greenhouse should be fixed in the config, not silently moved.

## Cell Centers

Each cell has a center coordinate. Device influence is calculated by distance from the device position to cell centers.

For cell `(0, 0, 0)` in the default grid, the center is:

```text
(0.5, 0.5, 0.375)
```

## Neighbors

The project uses a 6-neighbor model:

```text
left, right, front, back, below, above
```

This is enough for the simplified heat and humidity spread planned for the project. Diagonal neighbors are intentionally not used to keep the model understandable and stable for a one-week implementation.

## Boundary Cells

A boundary cell touches at least one outside surface:

- side wall;
- end wall;
- floor;
- roof.

Boundary cells will be important for heat loss and ventilation.

## Plants

Each plant control point is mapped to one grid cell. During simulation, plant metrics will read temperature and humidity from those cells.

## Devices

Each vent, heater, and humidifier has:

- an anchor cell, which contains the device position;
- influenced cells, found by radius;
- distance from the device to each influenced cell center;
- a simple influence weight.

The current influence weight is distance-based:

```text
weight = 1 - distance / radius
```

The value is clamped so edge cells still have a small non-zero influence. Later, Physics will use these cells and weights instead of searching the grid again every time step.

