# Control Comparison Example

This is the short scenario for comparing two greenhouse control strategies:

- `on_off`: simple sensor automation that switches devices fully on or off.
- `ml`: adaptive control that starts from the proportional controller and explores small adjustments.

Run:

```bash
./build/greenhouse3d compare examples/control_comparison/config.json
```

Windows PowerShell:

```powershell
.\build-mingw\greenhouse3d.exe compare examples\control_comparison\config.json
```

Output is written to:

```text
outputs/examples/control_comparison/
```

Important files:

- `comparison_report.txt` compares both strategies.
- `on_off/report.txt` describes the plain sensor automation run.
- `ml/report.txt` describes the ML-assisted run.
- `on_off/metrics.csv` and `ml/metrics.csv` can be plotted side by side.

Use this example when you want a quick answer to whether ML is worth using for
the same greenhouse, weather, devices, and plants.
