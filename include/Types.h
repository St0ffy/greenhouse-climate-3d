#pragma once

#include <string>
#include <vector>

namespace greenhouse {

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct GridSize {
    int nx = 1;
    int ny = 1;
    int nz = 1;
};

struct GreenhouseSize {
    double length = 1.0;
    double width = 1.0;
    double height = 1.0;
};

struct CellState {
    double temperatureC = 20.0;
    double humidityPercent = 60.0;
};

struct PlantPoint {
    std::string name;
    Vec3 position;
    double targetTemperatureC = 22.0;
};

} // namespace greenhouse

