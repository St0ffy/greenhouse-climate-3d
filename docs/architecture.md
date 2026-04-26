# Architecture

## Overview

The project is a terminal C++ simulator for simplified 3D greenhouse climate modeling.

The program reads a scenario from `configs/default_config.json`, builds a 3D grid, applies simplified physical rules over time, then writes CSV/JSON/text outputs.

## Main Flow

```text
main.cpp
  -> Config
  -> Geometry
  -> Weather
  -> Material
  -> Devices
  -> Simulator
       -> Physics
  -> Exporter
  -> Report
```

In optimization mode:

```text
main.cpp
  -> Config
  -> Optimizer
       -> Simulator
            -> Physics
  -> Exporter
  -> Report
```

## Module Responsibilities

### Config

Reads user settings and validates required values. It should not run the simulation and should not contain physical formulas.

### Geometry

Stores greenhouse size, grid dimensions, cell size, coordinate conversion, and neighbor lookup. It should not know about weather, heaters, or humidity formulas.

Current grid conventions:

- Coordinates are measured in meters.
- Valid coordinates are inside `[0, length] x [0, width] x [0, height]`.
- Cell indices are zero-based: `(x, y, z)`.
- Linear cell index is `x + nx * (y + ny * z)`.
- Direct physics neighbors use the 6-neighbor model: left, right, front, back, below, above.
- Boundary cells are cells touching any greenhouse wall, floor, or roof.

### Weather

Provides outside temperature, outside humidity, and solar radiation for a requested time. It may use constant config values or a CSV weather file.

### Material

Stores greenhouse cover parameters such as heat loss coefficient and solar transmission.

### Devices

Stores vents, heaters, humidifiers, and plant control points. It maps their real coordinates to grid cells with help from Geometry.

Day 2 adds device-to-grid mapping. Each plant has one control cell. Each vent, heater, and humidifier has an anchor cell plus a list of influenced cells found by radius around the device position. Influence cells also store distance and a simple distance-based weight for later physics formulas.

### Physics

Calculates one time step. It applies heat transfer, heat loss, solar heating, ventilation, heaters, and humidifiers.

### Simulator

Owns the time loop. It calls Physics repeatedly, collects history and metrics, and returns a simulation result.

### Optimizer

Runs several simulations with different heater positions and chooses the best option using a simple quality function.

### Exporter

Writes result files to `outputs/`. It should not change simulation data.

### Report

Creates a human-readable terminal and text report.

## Day 2 Status

The repository scaffold and geometry layer are prepared. The program can now build a 3D grid, map plants and devices to cells, show the mapping in the terminal, and run a small geometry test.

The physics and result-export modules are intentionally still placeholders and will be implemented step by step.
