#pragma once

#include "Config.h"
#include "Devices.h"
#include "Geometry.h"
#include "Material.h"
#include "Simulator.h"
#include "Weather.h"

#include <cstddef>
#include <string>
#include <vector>

namespace greenhouse {

struct OptimizationScore {
    double quality = 0.0;
    double averagePlantTemperatureErrorC = 0.0;
    double maxPlantTemperatureErrorC = 0.0;
    double energyKWh = 0.0;
};

struct OptimizationResult {
    SimulationResult baselineSimulation;
    SimulationResult bestSimulation;
    MappedDeviceSet baselineDevices;
    MappedDeviceSet bestDevices;
    OptimizationScore baselineScore;
    OptimizationScore bestScore;
    std::vector<Vec3> candidatePositions;
    std::size_t testedLayouts = 0;
    bool improved = false;
    std::string recommendation;
};

OptimizationScore scoreSimulation(
    const SimulationResult& simulation,
    double energyWeight
);

OptimizationResult optimizeHeaterPlacement(
    const SimulationConfig& config,
    const Grid3D& grid,
    const WeatherTimeline& weather,
    const MaterialProperties& material
);

} // namespace greenhouse
