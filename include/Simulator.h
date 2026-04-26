#pragma once

#include "Config.h"
#include "Devices.h"
#include "Geometry.h"
#include "Material.h"
#include "Physics.h"
#include "Weather.h"

#include <vector>

namespace greenhouse {

struct SimulationFrame {
    double timeSeconds = 0.0;
    WeatherCondition weather;
    ClimateStepSummary climate;
    PlantTemperatureStats plantTemperature;
    PlantHumidityStats plantHumidity;
    double cumulativeHeaterEnergyKWh = 0.0;
    std::vector<CellState> cells;
};

struct SimulationResult {
    SimulationConfig config;
    MaterialProperties material;
    std::vector<SimulationFrame> frames;
    double durationSeconds = 0.0;
    double timeStepSeconds = 0.0;
    double totalHeaterEnergyKWh = 0.0;
};

SimulationResult runSimulation(
    const SimulationConfig& config,
    const Grid3D& grid,
    const WeatherTimeline& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices,
    const ClimatePhysicsSettings& settings = ClimatePhysicsSettings{}
);

} // namespace greenhouse
