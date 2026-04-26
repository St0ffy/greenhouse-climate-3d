#pragma once

#include "Types.h"

#include <string>

namespace greenhouse {

struct SimulationConfig {
    std::string mode = "simulate";
    GreenhouseSize greenhouseSize;
    GridSize gridSize;
    double durationSeconds = 3600.0;
    double timeStepSeconds = 60.0;
    double initialTemperatureC = 20.0;
    double initialHumidityPercent = 60.0;
    double outsideTemperatureC = 5.0;
    double outsideHumidityPercent = 80.0;
    double solarRadiationWm2 = 300.0;
    bool humidityEnabled = true;
    std::string humidifierMode = "medium";
    double heaterPowerW = 1000.0;
    std::vector<PlantPoint> plants;
};

SimulationConfig loadConfig(const std::string& path);

} // namespace greenhouse

