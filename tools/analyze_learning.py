#!/usr/bin/env python3
"""Analyze greenhouse ML-controller learning from metrics.csv."""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Iterable

try:
    import pandas as pd
except ImportError:
    print("Missing dependency: pandas. Install with: pip install pandas matplotlib", file=sys.stderr)
    sys.exit(1)

try:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:
    print("Missing dependency: matplotlib. Install with: pip install pandas matplotlib", file=sys.stderr)
    sys.exit(1)


REWARD_COLUMNS = ["controller_reward", "reward", "ml_reward"]
COMFORT_COLUMNS = [
    "average_comfort",
    "avg_comfort",
    "plant_comfort",
    "average_plant_comfort",
    "plant_avg_comfort",
]
HEALTH_COLUMNS = [
    "average_health",
    "avg_health",
    "plant_health",
    "average_plant_health",
    "plant_avg_health",
]
TEMP_ERROR_COLUMNS = [
    "avg_temperature_error",
    "average_temperature_error",
    "temperature_error",
    "plant_avg_temperature_error_c",
    "plant_average_temperature_error_c",
]
HUMIDITY_ERROR_COLUMNS = [
    "avg_humidity_error",
    "average_humidity_error",
    "humidity_error",
    "plant_avg_humidity_error",
]
CUMULATIVE_ENERGY_COLUMNS = [
    "total_energy_kwh",
    "energy_kwh",
    "cumulative_energy_kwh",
    "cumulative_device_energy_kwh",
    "cumulative_heater_energy_kwh",
]
STEP_ENERGY_COLUMNS = ["step_energy_kwh", "device_step_energy_kwh"]


def warn(message: str) -> None:
    print(f"warning: {message}", file=sys.stderr)


def find_column(df: pd.DataFrame, candidates: Iterable[str], label: str | None = None) -> str | None:
    for candidate in candidates:
        if candidate in df.columns:
            return candidate
    if label:
        warn(f"missing column for {label}: tried {', '.join(candidates)}")
    return None


def numeric_series(df: pd.DataFrame, column: str | None) -> pd.Series | None:
    if column is None:
        return None
    return pd.to_numeric(df[column], errors="coerce")


def rolling(series: pd.Series) -> pd.Series:
    window = max(2, min(30, max(2, len(series) // 20)))
    return series.rolling(window=window, min_periods=1).mean()


def simple_slope(x: pd.Series, y: pd.Series) -> float | None:
    data = pd.DataFrame({"x": x, "y": y}).dropna()
    if len(data) < 2:
        return None
    x_values = data["x"].astype(float)
    y_values = data["y"].astype(float)
    x_mean = x_values.mean()
    y_mean = y_values.mean()
    denominator = ((x_values - x_mean) ** 2).sum()
    if denominator == 0:
        return None
    return float(((x_values - x_mean) * (y_values - y_mean)).sum() / denominator)


def save_line_plot(
    path: Path,
    x: pd.Series,
    x_label: str,
    title: str,
    lines: list[tuple[str, pd.Series]],
) -> bool:
    plotted = False
    plt.figure(figsize=(11, 6))
    for label, series in lines:
        if series is None:
            continue
        data = pd.DataFrame({"x": x, "y": series}).dropna()
        if data.empty:
            continue
        plt.plot(data["x"], data["y"], label=label)
        plotted = True

    if not plotted:
        plt.close()
        return False

    plt.title(title)
    plt.xlabel(x_label)
    plt.ylabel("value")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(path)
    plt.close()
    return True


def detect_time(df: pd.DataFrame) -> tuple[pd.Series, str, pd.Series | None]:
    for column in ["time_seconds", "simulation_time_seconds"]:
        if column in df.columns:
            hours = pd.to_numeric(df[column], errors="coerce") / 3600.0
            return hours, "time, hours", hours
    if "time" in df.columns:
        hours = pd.to_numeric(df["time"], errors="coerce") / 3600.0
        return hours, "time, hours", hours
    if "step" in df.columns:
        step = pd.to_numeric(df["step"], errors="coerce")
        if "time_step_seconds" in df.columns:
            hours = step * pd.to_numeric(df["time_step_seconds"], errors="coerce") / 3600.0
            return hours, "time, hours", hours
        warn("using step as X axis because no time column was found")
        return step, "step", None

    warn("no time column found; using row index as X axis")
    index = pd.Series(range(len(df)), index=df.index, dtype=float)
    return index, "row", None


def energy_for_rows(df: pd.DataFrame, cumulative_col: str | None, step_col: str | None) -> pd.Series | None:
    if step_col:
        return pd.to_numeric(df[step_col], errors="coerce")
    if cumulative_col:
        cumulative = pd.to_numeric(df[cumulative_col], errors="coerce")
        return cumulative.diff().fillna(cumulative)
    return None


def total_energy(group: pd.DataFrame, cumulative_col: str | None, step_col: str | None) -> float | None:
    if step_col:
        return float(pd.to_numeric(group[step_col], errors="coerce").sum())
    if cumulative_col:
        values = pd.to_numeric(group[cumulative_col], errors="coerce").dropna()
        if values.empty:
            return None
        return float(values.max() - values.min())
    return None


def mean_or_none(group: pd.DataFrame, column: str | None) -> float | None:
    if not column:
        return None
    values = pd.to_numeric(group[column], errors="coerce").dropna()
    if values.empty:
        return None
    return float(values.mean())


def write_comparison_csv(rows: list[dict], path: Path) -> pd.DataFrame:
    comparison = pd.DataFrame(rows)
    comparison.to_csv(path, index=False)
    return comparison


def comparison_plot(path: Path, df: pd.DataFrame, x_col: str, title: str) -> bool:
    metric_columns = [
        column
        for column in [
            "mean_reward",
            "mean_comfort",
            "total_energy_kwh",
            "mean_temperature_error",
            "mean_humidity_error",
        ]
        if column in df.columns and df[column].notna().any()
    ]
    if df.empty or not metric_columns:
        warn(f"not enough data for {path.name}")
        return False

    fig, axes = plt.subplots(len(metric_columns), 1, figsize=(11, 3.2 * len(metric_columns)), sharex=True)
    if len(metric_columns) == 1:
        axes = [axes]
    for axis, column in zip(axes, metric_columns):
        axis.plot(df[x_col], df[column], marker="o", label=column)
        axis.set_ylabel(column)
        axis.grid(True, alpha=0.3)
        axis.legend()
    axes[-1].set_xlabel(x_col)
    fig.suptitle(title)
    fig.tight_layout()
    fig.savefig(path)
    plt.close(fig)
    return True


def day_comparison(
    df: pd.DataFrame,
    out_dir: Path,
    hours: pd.Series | None,
    columns: dict[str, str | None],
) -> pd.DataFrame:
    working = df.copy()
    if hours is not None:
        working["_day"] = (hours.fillna(0).apply(math.floor) // 24 + 1).astype(int)
    else:
        working["_day"] = 1

    rows = []
    for day, group in working.groupby("_day", sort=True):
        rows.append(
            {
                "day": int(day),
                "rows_count": int(len(group)),
                "mean_reward": mean_or_none(group, columns["reward"]),
                "mean_comfort": mean_or_none(group, columns["comfort"]),
                "mean_health": mean_or_none(group, columns["health"]),
                "total_energy_kwh": total_energy(group, columns["cumulative_energy"], columns["step_energy"]),
                "mean_temperature_error": mean_or_none(group, columns["temperature_error"]),
                "mean_humidity_error": mean_or_none(group, columns["humidity_error"]),
            }
        )

    comparison = write_comparison_csv(rows, out_dir / "day_comparison.csv")
    comparison_plot(out_dir / "day_comparison.png", comparison, "day", "Day comparison")
    return comparison


def window_comparison(
    df: pd.DataFrame,
    out_dir: Path,
    hours: pd.Series | None,
    window_hours: float,
    columns: dict[str, str | None],
) -> pd.DataFrame:
    working = df.copy()
    if hours is not None and hours.notna().any():
        duration = float(hours.max() - hours.min())
        if duration <= 0 or duration < window_hours:
            parts = min(4, max(1, len(working)))
            working["_window"] = pd.qcut(range(len(working)), q=parts, labels=False, duplicates="drop")
        else:
            start = float(hours.min())
            working["_window"] = ((hours - start) / max(window_hours, 1e-9)).apply(math.floor).astype(int)
        working["_hours"] = hours
    else:
        parts = min(4, max(1, len(working)))
        working["_window"] = pd.qcut(range(len(working)), q=parts, labels=False, duplicates="drop")
        working["_hours"] = pd.Series(range(len(working)), index=working.index, dtype=float)

    rows = []
    for window, group in working.groupby("_window", sort=True):
        rows.append(
            {
                "window_index": int(window) + 1,
                "start_hour": float(group["_hours"].min()),
                "end_hour": float(group["_hours"].max()),
                "rows_count": int(len(group)),
                "mean_reward": mean_or_none(group, columns["reward"]),
                "mean_comfort": mean_or_none(group, columns["comfort"]),
                "mean_health": mean_or_none(group, columns["health"]),
                "total_energy_kwh": total_energy(group, columns["cumulative_energy"], columns["step_energy"]),
                "mean_temperature_error": mean_or_none(group, columns["temperature_error"]),
                "mean_humidity_error": mean_or_none(group, columns["humidity_error"]),
            }
        )

    comparison = write_comparison_csv(rows, out_dir / "window_comparison.csv")
    comparison_plot(
        out_dir / "window_comparison.png",
        comparison,
        "window_index",
        "Window comparison",
    )
    return comparison


def third_means(series: pd.Series | None) -> tuple[float | None, float | None]:
    if series is None or series.dropna().empty:
        return None, None
    count = len(series)
    third = max(1, count // 3)
    return float(series.iloc[:third].mean()), float(series.iloc[-third:].mean())


def relative_change(first: float | None, last: float | None) -> float | None:
    if first is None or last is None:
        return None
    denominator = abs(first) if abs(first) > 1e-9 else 1.0
    return (last - first) / denominator


def build_report(
    input_file: Path,
    out_dir: Path,
    df: pd.DataFrame,
    x: pd.Series,
    hours: pd.Series | None,
    columns: dict[str, str | None],
) -> None:
    reward = numeric_series(df, columns["reward"])
    comfort = numeric_series(df, columns["comfort"])
    health = numeric_series(df, columns["health"])
    temp_error = numeric_series(df, columns["temperature_error"])
    energy_step = energy_for_rows(df, columns["cumulative_energy"], columns["step_energy"])

    reward_first, reward_last = third_means(reward)
    comfort_first, comfort_last = third_means(comfort)
    health_first, health_last = third_means(health)
    temp_first, temp_last = third_means(temp_error)

    if energy_step is not None:
        energy_first, energy_last = third_means(energy_step)
        if hours is not None and len(hours.dropna()) >= 2:
            duration_hours = max(float(hours.max() - hours.min()), 1e-9)
            frame_hours = duration_hours / max(1, len(df) - 1)
            energy_per_hour_first = None if energy_first is None else energy_first / frame_hours
            energy_per_hour_last = None if energy_last is None else energy_last / frame_hours
        else:
            energy_per_hour_first = energy_first
            energy_per_hour_last = energy_last
    else:
        energy_per_hour_first = None
        energy_per_hour_last = None

    reward_slope = simple_slope(x, reward) if reward is not None else None
    comfort_slope = simple_slope(x, comfort) if comfort is not None else None

    reward_change = relative_change(reward_first, reward_last)
    comfort_change = relative_change(comfort_first, comfort_last)
    energy_change = relative_change(energy_per_hour_first, energy_per_hour_last)

    if reward_change is None:
        conclusion = "Not enough reward data to judge ML learning."
    elif reward_change > 0.03 and (comfort_change is None or comfort_change > 0.01):
        if energy_change is not None and energy_change > 0.25:
            conclusion = "ML improved comfort/reward but uses more energy."
        else:
            conclusion = "ML likely improved control quality."
    elif abs(reward_change) <= 0.03:
        conclusion = "ML learning effect is weak or not visible in this run."
    else:
        conclusion = "ML did not improve in this run or exploration is too high."

    duration_hours = float(hours.max() - hours.min()) if hours is not None and hours.notna().any() else None
    duration_days = None if duration_hours is None else duration_hours / 24.0
    notes = []
    if duration_days is not None and duration_days < 1.0:
        notes.append("Run is short, conclusion is approximate.")
    if duration_days is not None and duration_days < 3.0:
        notes.append("Use window comparison because day-by-day comparison is limited.")

    summary = {
        "input_file": str(input_file),
        "duration_hours": duration_hours,
        "duration_days": duration_days,
        "rows": int(len(df)),
        "reward": {
            "first_third_mean": reward_first,
            "last_third_mean": reward_last,
            "trend_slope": reward_slope,
        },
        "comfort": {
            "first_third_mean": comfort_first,
            "last_third_mean": comfort_last,
            "trend_slope": comfort_slope,
        },
        "health": {
            "first_third_mean": health_first,
            "last_third_mean": health_last,
        },
        "energy": {
            "energy_per_hour_first": energy_per_hour_first,
            "energy_per_hour_last": energy_per_hour_last,
        },
        "temperature_error": {
            "first_third_mean": temp_first,
            "last_third_mean": temp_last,
        },
        "conclusion": conclusion,
        "notes": notes,
    }

    (out_dir / "learning_summary.json").write_text(
        json.dumps(summary, indent=2),
        encoding="utf-8",
    )

    lines = [
        "Greenhouse ML learning report",
        "=============================",
        f"Input file: {input_file}",
        f"Rows: {len(df)}",
        f"Duration hours: {duration_hours if duration_hours is not None else 'unknown'}",
        "",
        f"Reward first third mean: {reward_first}",
        f"Reward last third mean: {reward_last}",
        f"Reward trend slope: {reward_slope}",
        f"Comfort first third mean: {comfort_first}",
        f"Comfort last third mean: {comfort_last}",
        f"Comfort trend slope: {comfort_slope}",
        f"Health first third mean: {health_first}",
        f"Health last third mean: {health_last}",
        f"Energy per hour first third: {energy_per_hour_first}",
        f"Energy per hour last third: {energy_per_hour_last}",
        f"Temperature error first third mean: {temp_first}",
        f"Temperature error last third mean: {temp_last}",
        "",
        f"Conclusion: {conclusion}",
    ]
    if notes:
        lines.append("")
        lines.extend(notes)

    (out_dir / "learning_report.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")


def make_plots(
    df: pd.DataFrame,
    out_dir: Path,
    x: pd.Series,
    x_label: str,
    columns: dict[str, str | None],
) -> None:
    reward = numeric_series(df, columns["reward"])
    if reward is not None:
        save_line_plot(
            out_dir / "ml_reward_over_time.png",
            x,
            x_label,
            "ML reward over time",
            [("controller_reward", reward), ("rolling average", rolling(reward))],
        )

    comfort = numeric_series(df, columns["comfort"])
    if comfort is not None:
        save_line_plot(
            out_dir / "plant_comfort_over_time.png",
            x,
            x_label,
            "Average plant comfort over time",
            [("comfort", comfort), ("rolling average", rolling(comfort))],
        )

    health = numeric_series(df, columns["health"])
    if health is not None:
        save_line_plot(
            out_dir / "plant_health_over_time.png",
            x,
            x_label,
            "Plant health over time",
            [("health", health), ("rolling average", rolling(health))],
        )

    energy_lines = []
    step_energy = numeric_series(df, columns["step_energy"])
    cumulative_energy = numeric_series(df, columns["cumulative_energy"])
    if step_energy is not None:
        energy_lines.append(("step energy kWh", step_energy))
        energy_lines.append(("step energy cumsum kWh", step_energy.fillna(0).cumsum()))
    if cumulative_energy is not None:
        energy_lines.append(("cumulative energy kWh", cumulative_energy))
    if energy_lines:
        save_line_plot(
            out_dir / "energy_over_time.png",
            x,
            x_label,
            "Energy over time",
            energy_lines,
        )

    temp_error = numeric_series(df, columns["temperature_error"])
    humidity_error = numeric_series(df, columns["humidity_error"])
    error_lines = []
    if temp_error is not None:
        error_lines.append(("temperature error", temp_error))
    if humidity_error is not None:
        error_lines.append(("humidity error", humidity_error))
    if error_lines:
        save_line_plot(
            out_dir / "climate_errors_over_time.png",
            x,
            x_label,
            "Temperature and humidity errors over time",
            error_lines,
        )

    summary_lines = []
    for label, series in [
        ("reward rolling", rolling(reward) if reward is not None else None),
        ("comfort rolling", rolling(comfort) if comfort is not None else None),
        ("energy per step rolling", rolling(step_energy) if step_energy is not None else None),
    ]:
        if series is None:
            continue
        min_value = series.min()
        max_value = series.max()
        if pd.isna(min_value) or pd.isna(max_value) or abs(max_value - min_value) < 1e-12:
            normalized = series * 0.0
        else:
            normalized = (series - min_value) / (max_value - min_value)
        summary_lines.append((label, normalized))
    if summary_lines:
        save_line_plot(
            out_dir / "ml_learning_summary.png",
            x,
            x_label,
            "ML learning summary, normalized rolling values",
            summary_lines,
        )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Analyze greenhouse ML learning from metrics.csv")
    parser.add_argument("metrics_csv", help="Path to outputs/metrics.csv")
    parser.add_argument("--out", default="outputs/plots", help="Output directory for plots and reports")
    parser.add_argument("--window-hours", type=float, default=6.0, help="Window size for window comparison")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    input_file = Path(args.metrics_csv)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    if not input_file.exists():
        print(f"metrics file not found: {input_file}", file=sys.stderr)
        return 1

    df = pd.read_csv(input_file)
    if df.empty:
        print(f"metrics file is empty: {input_file}", file=sys.stderr)
        return 1

    x, x_label, hours = detect_time(df)
    columns = {
        "reward": find_column(df, REWARD_COLUMNS, "ML reward"),
        "comfort": find_column(df, COMFORT_COLUMNS, "plant comfort"),
        "health": find_column(df, HEALTH_COLUMNS, "plant health"),
        "temperature_error": find_column(df, TEMP_ERROR_COLUMNS, "temperature error"),
        "humidity_error": find_column(df, HUMIDITY_ERROR_COLUMNS, "humidity error"),
        "cumulative_energy": find_column(df, CUMULATIVE_ENERGY_COLUMNS, "cumulative energy"),
        "step_energy": find_column(df, STEP_ENERGY_COLUMNS),
    }

    make_plots(df, out_dir, x, x_label, columns)
    day_comparison(df, out_dir, hours, columns)
    window_comparison(df, out_dir, hours, max(0.01, args.window_hours), columns)
    build_report(input_file, out_dir, df, x, hours, columns)

    print(f"Learning analysis written to {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
