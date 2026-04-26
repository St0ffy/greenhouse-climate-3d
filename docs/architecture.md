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

### Weather

Provides outside temperature, outside humidity, and solar radiation for a requested time. It may use constant config values or a CSV weather file.

### Material

Stores greenhouse cover parameters such as heat loss coefficient and solar transmission.

### Devices

Stores vents, heaters, humidifiers, and plant control points. It maps their real coordinates to grid cells with help from Geometry.

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

## Day 1 Status

The repository scaffold is prepared. The physics and data-processing modules are intentionally placeholders and will be implemented step by step.

