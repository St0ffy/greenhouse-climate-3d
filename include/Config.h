#pragma once

#include "Types.h"

#include <string>
#include <vector>

namespace greenhouse {

struct SimulationConfig {
    std::string mode = "simulate";
    GreenhouseSize greenhouseSize;
    GridSize gridSize;
    double durationSeconds = 3600.0;
    double timeStepSeconds = 60.0;
    double initialTemperatureC = 20.0;
    double initialHumidityPercent = 60.0;
    WeatherSpec weather;
    MaterialSpec material;
    bool humidityEnabled = true;
    std::string humidifierMode = "medium";
    double heaterPowerW = 1000.0;
    std::vector<VentSpec> vents;
    std::vector<HeaterSpec> heaters;
    std::vector<HumidifierSpec> humidifiers;
    std::vector<PlantPoint> plants;
    OutputSpec output;
};

SimulationConfig loadConfig(const std::string& path);

} // namespace greenhouse
