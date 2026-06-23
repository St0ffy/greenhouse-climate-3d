#pragma once

#include "Config.h"
#include "Devices.h"
#include "Geometry.h"
#include "Material.h"
#include "Simulator.h"
#include "Weather.h"

#include <string>

namespace greenhouse {

struct ControlComparisonSummary {
    bool targetReached = false;
    double targetReachedSeconds = -1.0;
    double finalAverageTemperatureErrorC = 0.0;
    double finalMaxTemperatureErrorC = 0.0;
    double finalAverageHumidityErrorPercent = 0.0;
    double finalMaxHumidityErrorPercent = 0.0;
    double finalAverageComfort = 0.0;
    double finalAverageHealth = 0.0;
    double finalMinHealth = 0.0;
    int finalAlivePlants = 0;
    double totalHeaterEnergyKWh = 0.0;
    double totalDeviceEnergyKWh = 0.0;
};

struct ControlComparisonRun {
    std::string label;
    SimulationResult result;
    ControlComparisonSummary summary;
};

struct ControlComparisonResult {
    ControlComparisonRun onOff;
    ControlComparisonRun ml;
    double onOffQuality = 0.0;
    double mlQuality = 0.0;
    std::string recommendation;
};

ControlComparisonSummary summarizeControlComparisonRun(
    const SimulationResult& result
);

ControlComparisonResult runControlComparison(
    const SimulationConfig& baseConfig,
    const Grid3D& grid,
    const WeatherTimeline& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices
);

} // namespace greenhouse
