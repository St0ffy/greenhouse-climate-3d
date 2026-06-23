#include "Comparison.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace greenhouse {

namespace {

std::string appendOutputSubdirectory(
    const std::string& baseDirectory,
    const std::string& subdirectory
) {
    const std::filesystem::path base =
        baseDirectory.empty()
            ? std::filesystem::path("outputs")
            : std::filesystem::path(baseDirectory);
    return (base / subdirectory).string();
}

SimulationConfig makeComparisonConfig(
    const SimulationConfig& baseConfig,
    const std::string& label,
    const std::string& strategy,
    bool mlEnabled
) {
    SimulationConfig config = baseConfig;
    config.mode = "compare:" + label;
    config.control.enabled = true;
    config.control.strategy = strategy;
    config.control.mlEnabled = mlEnabled;
    config.control.mlMemoryEnabled = mlEnabled;
    config.control.mlMemoryLogEnabled = false;
    config.output.directory =
        appendOutputSubdirectory(baseConfig.output.directory, label);
    config.output.terminalView.enabled = false;
    config.output.terminalView.interactive = false;
    config.output.terminalView.loopPlayback = false;
    return config;
}

std::pair<double, double> humidityErrorStats(const SimulationFrame& frame) {
    const std::size_t count =
        std::min(frame.devices.plants.size(), frame.sensorReadings.size());
    if (count == 0) {
        return {0.0, 0.0};
    }

    double totalError = 0.0;
    double maxError = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        const double error = std::fabs(
            frame.sensorReadings[i].humidityPercent
            - frame.devices.plants[i].plant.targetHumidityPercent
        );
        totalError += error;
        maxError = std::max(maxError, error);
    }

    return {totalError / static_cast<double>(count), maxError};
}

bool frameReachedTarget(
    const SimulationFrame& frame,
    double temperatureToleranceC,
    double humidityTolerancePercent
) {
    const std::size_t count =
        std::min(frame.devices.plants.size(), frame.sensorReadings.size());
    if (count == 0) {
        return false;
    }

    const double temperatureTolerance = std::max(0.0, temperatureToleranceC);
    const double humidityTolerance = std::max(0.0, humidityTolerancePercent);
    for (std::size_t i = 0; i < count; ++i) {
        const PlantPoint& plant = frame.devices.plants[i].plant;
        const PlantSensorReading& sensor = frame.sensorReadings[i];
        if (std::fabs(sensor.temperatureC - plant.targetTemperatureC)
                > temperatureTolerance
            || std::fabs(sensor.humidityPercent - plant.targetHumidityPercent)
                > humidityTolerance) {
            return false;
        }
    }

    return true;
}

double comparisonQuality(const ControlComparisonSummary& summary) {
    const double targetPenalty = summary.targetReached ? 0.0 : 3.0;
    return summary.finalAverageTemperatureErrorC
        + 0.05 * summary.finalAverageHumidityErrorPercent
        + 2.0 * (1.0 - summary.finalAverageComfort)
        + 0.05 * summary.totalDeviceEnergyKWh
        + targetPenalty;
}

std::string buildRecommendation(
    double onOffQuality,
    double mlQuality
) {
    constexpr double meaningfulDifference = 0.05;
    if (mlQuality + meaningfulDifference < onOffQuality) {
        return "ML control is recommended for this scenario: it gives the lower combined plant comfort, climate error, and energy score.";
    }
    if (onOffQuality + meaningfulDifference < mlQuality) {
        return "On/off sensor automation is recommended for this scenario: it gives the lower combined plant comfort, climate error, and energy score.";
    }
    return "Both strategies are close in this scenario; prefer on/off automation for simplicity or ML control when longer adaptation runs are planned.";
}

} // namespace

ControlComparisonSummary summarizeControlComparisonRun(
    const SimulationResult& result
) {
    ControlComparisonSummary summary;
    summary.totalHeaterEnergyKWh = result.totalHeaterEnergyKWh;
    summary.totalDeviceEnergyKWh = result.totalDeviceEnergyKWh;

    for (const SimulationFrame& frame : result.frames) {
        if (frameReachedTarget(
                frame,
                result.config.control.temperatureToleranceC,
                result.config.control.humidityTolerancePercent
            )) {
            summary.targetReached = true;
            summary.targetReachedSeconds = frame.timeSeconds;
            break;
        }
    }

    if (result.frames.empty()) {
        return summary;
    }

    const SimulationFrame& final = result.frames.back();
    const std::pair<double, double> humidityErrors = humidityErrorStats(final);
    summary.finalAverageTemperatureErrorC =
        final.plantTemperature.averageAbsoluteErrorC;
    summary.finalMaxTemperatureErrorC =
        final.plantTemperature.maxAbsoluteErrorC;
    summary.finalAverageHumidityErrorPercent = humidityErrors.first;
    summary.finalMaxHumidityErrorPercent = humidityErrors.second;
    summary.finalAverageComfort = final.plantGrowth.averageComfort;
    summary.finalAverageHealth = final.plantGrowth.averageHealth;
    summary.finalMinHealth = final.plantGrowth.minHealth;
    summary.finalAlivePlants = final.plantGrowth.aliveCount;
    return summary;
}

ControlComparisonResult runControlComparison(
    const SimulationConfig& baseConfig,
    const Grid3D& grid,
    const WeatherTimeline& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices
) {
    ControlComparisonResult comparison;

    const SimulationConfig onOffConfig =
        makeComparisonConfig(baseConfig, "on_off", "on_off", false);
    const SimulationConfig mlConfig =
        makeComparisonConfig(baseConfig, "ml", "proportional", true);

    comparison.onOff.label = "on_off";
    comparison.onOff.result =
        runSimulation(onOffConfig, grid, weather, material, devices);
    comparison.onOff.summary =
        summarizeControlComparisonRun(comparison.onOff.result);

    comparison.ml.label = "ml";
    comparison.ml.result =
        runSimulation(mlConfig, grid, weather, material, devices);
    comparison.ml.summary =
        summarizeControlComparisonRun(comparison.ml.result);

    comparison.onOffQuality = comparisonQuality(comparison.onOff.summary);
    comparison.mlQuality = comparisonQuality(comparison.ml.summary);
    comparison.recommendation =
        buildRecommendation(comparison.onOffQuality, comparison.mlQuality);
    return comparison;
}

} // namespace greenhouse
