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

std::vector<CellState> makeInitialCells(
    const Grid3D& grid,
    double temperatureC,
    double humidityPercent
);

TemperatureStats summarizeTemperature(const std::vector<CellState>& cells);

PlantTemperatureStats summarizePlantTemperatures(
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

} // namespace greenhouse
