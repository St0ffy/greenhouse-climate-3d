#pragma once

#include "Devices.h"
#include "Geometry.h"
#include "Material.h"
#include "Types.h"
#include "Weather.h"

#include <vector>

namespace greenhouse {

struct TemperaturePhysicsSettings {
    double heatTransferRatePerSecond = 0.0012;
    double boundaryHeatLossRatePerSecond = 0.0009;
    double solarGainCPerWm2Second = 0.000012;
    double heaterGainCPerWattSecond = 0.000070;
    double minTemperatureC = -80.0;
    double maxTemperatureC = 100.0;
};

struct TemperatureStats {
    double minTemperatureC = 0.0;
    double maxTemperatureC = 0.0;
    double averageTemperatureC = 0.0;
};

struct PlantTemperatureStats {
    double averageTemperatureC = 0.0;
    double averageTargetTemperatureC = 0.0;
    double averageAbsoluteErrorC = 0.0;
    double maxAbsoluteErrorC = 0.0;
};

struct TemperatureStepSummary {
    TemperatureStats temperature;
    double averageNeighborExchangeAbsC = 0.0;
    double averageBoundaryLossC = 0.0;
    double averageSolarGainC = 0.0;
    double averageHeaterGainC = 0.0;
    double heaterEnergyKWh = 0.0;
};

struct TemperatureStepResult {
    std::vector<CellState> cells;
    TemperatureStepSummary summary;
};

struct HumidityPhysicsSettings {
    bool humidityEnabled = true;
    double humidityTransferRatePerSecond = 0.0009;
    double ventTemperatureExchangeRatePerSecond = 0.0030;
    double ventHumidityExchangeRatePerSecond = 0.0040;
    double maxVentilationMixPerStep = 0.35;
    double humidifierGainPercentPerSecond = 0.030;
};

struct ClimatePhysicsSettings {
    TemperaturePhysicsSettings temperature;
    HumidityPhysicsSettings humidity;
};

struct HumidityStats {
    double minHumidityPercent = 0.0;
    double maxHumidityPercent = 0.0;
    double averageHumidityPercent = 0.0;
};

struct PlantHumidityStats {
    double averageHumidityPercent = 0.0;
    double minHumidityPercent = 0.0;
    double maxHumidityPercent = 0.0;
};

struct ClimateStepSummary {
    TemperatureStepSummary temperatureStep;
    HumidityStats humidity;
    double averageHumidityExchangeAbsPercent = 0.0;
    double averageHumidifierGainPercent = 0.0;
    double averageVentTemperatureDeltaC = 0.0;
    double averageVentHumidityDeltaPercent = 0.0;
};

struct ClimateStepResult {
    std::vector<CellState> cells;
    ClimateStepSummary summary;
};

std::vector<CellState> makeInitialCells(
    const Grid3D& grid,
    double temperatureC,
    double humidityPercent
);

TemperatureStats summarizeTemperature(const std::vector<CellState>& cells);
HumidityStats summarizeHumidity(const std::vector<CellState>& cells);

PlantTemperatureStats summarizePlantTemperatures(
    const std::vector<CellState>& cells,
    const std::vector<MappedPlantPoint>& plants
);

PlantHumidityStats summarizePlantHumidity(
    const std::vector<CellState>& cells,
    const std::vector<MappedPlantPoint>& plants
);

TemperatureStepResult advanceTemperature(
    const std::vector<CellState>& current,
    const Grid3D& grid,
    const WeatherCondition& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices,
    double timeStepSeconds,
    const TemperaturePhysicsSettings& settings = TemperaturePhysicsSettings{}
);

ClimateStepResult advanceClimate(
    const std::vector<CellState>& current,
    const Grid3D& grid,
    const WeatherCondition& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices,
    double timeStepSeconds,
    const ClimatePhysicsSettings& settings = ClimatePhysicsSettings{}
);

} // namespace greenhouse
