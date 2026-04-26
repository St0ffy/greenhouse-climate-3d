# User Guide

This project is a terminal C++ simulator. It is designed to work locally on Windows and on a Linux server through PuTTY.

## 1. Get The Project

On Windows, work in the project folder:

```powershell
cd "C:\Users\usr\Documents\New project"
```

On a Linux server through PuTTY:

```bash
git clone https://github.com/St0ffy/greenhouse-climate-3d.git
cd greenhouse-climate-3d
```

If the project is already cloned:

```bash
git pull
```

## 2. Build

Preferred CMake build:

```bash
cmake -S . -B build
cmake --build build
```

If CMake is not available but `g++` is available:

```bash
g++ -std=c++17 -I include src/main.cpp src/Config.cpp src/Geometry.cpp src/Devices.cpp src/Weather.cpp src/Material.cpp src/Physics.cpp src/Simulator.cpp src/Exporter.cpp src/Report.cpp src/Optimizer.cpp -o greenhouse3d
```

On Windows with `g++`:

```powershell
g++ -std=c++17 -I include src/main.cpp src/Config.cpp src/Geometry.cpp src/Devices.cpp src/Weather.cpp src/Material.cpp src/Physics.cpp src/Simulator.cpp src/Exporter.cpp src/Report.cpp src/Optimizer.cpp -o greenhouse3d.exe
```

## 3. Configure

The main config is:

```text
configs/default_config.json
```

The user can change:

- greenhouse size;
- 3D grid size;
- simulation duration;
- time step;
- initial temperature and humidity;
- outside weather or weather CSV file;
- material coefficients;
- vent positions and opening;
- heater positions, power, and influence radius;
- humidifier mode;
- plant control points and target temperature;
- output directory;
- optimizer limits.

## 4. Run Normal Simulation

Linux/PuTTY:

```bash
./build/greenhouse3d simulate configs/default_config.json
```

Direct `g++` build:

```bash
./greenhouse3d simulate configs/default_config.json
```

Windows PowerShell:

```powershell
.\greenhouse3d.exe simulate configs\default_config.json
```

What happens:

1. Program reads config.
2. Builds the 3D grid.
3. Maps plants and devices to cells.
4. Loads weather.
5. Runs the full time simulation.
6. Prints a report in the terminal.
7. Saves output files.

## 5. Run Heater Optimization

Linux/PuTTY:

```bash
./build/greenhouse3d optimize configs/default_config.json
```

Windows PowerShell:

```powershell
.\greenhouse3d.exe optimize configs\default_config.json
```

What happens:

1. Program runs the baseline simulation.
2. Generates candidate heater positions.
3. Tests heater layouts.
4. Scores each layout.
5. Prints recommended heater coordinates.
6. Exports the best simulation result.

If this mode is slow, lower:

```json
"max_candidates": 20,
"max_layouts": 200
```

## 6. Read Outputs

Output files are written to:

```text
outputs/
```

Files:

- `report.txt` - readable terminal-style report.
- `summary.json` - compact final summary.
- `metrics.csv` - one row per simulation frame.
- `cell_history.csv` - every cell in every frame.

Use `metrics.csv` for charts. Use `cell_history.csv` for external 3D visualization or slices.

## 7. Work With GitHub And PuTTY

Before working:

```bash
git pull
```

After changes:

```bash
git add .
git commit -m "Describe changes"
git push
```

Recommended workflow:

- Codex/Windows side changes code and pushes.
- PuTTY/Linux side runs `git pull`, builds, tests, and runs simulations.
- If errors appear in PuTTY, copy the terminal output back into the chat.

## User Capabilities

The user can:

- run a full 3D greenhouse climate simulation;
- configure geometry, weather, material, devices, and plant points;
- enable or disable humidity simulation;
- model vents, heaters, and humidifiers;
- inspect plant-zone temperature error;
- estimate heater energy use;
- export CSV and JSON results;
- use external tools for visualization;
- run heater placement optimization;
- compare baseline and optimized heater layouts;
- work locally on Windows and remotely through PuTTY using GitHub.

