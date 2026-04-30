#pragma once

#include <string>
#include <vector>

namespace greenhouse {

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct GridIndex {
    int x = 0;
    int y = 0;
    int z = 0;
};

inline bool operator==(const GridIndex& left, const GridIndex& right) {
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

inline bool operator!=(const GridIndex& left, const GridIndex& right) {
    return !(left == right);
}

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

struct WeatherSpec {
    double outsideTemperatureC = 5.0;
    double outsideHumidityPercent = 80.0;
    double solarRadiationWm2 = 300.0;
    std::string weatherFile;
};

struct MaterialSpec {
    std::string name = "polycarbonate";
    double heatLossCoefficient = -1.0;
    double solarTransmission = -1.0;
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

struct VentSpec {
    std::string name;
    Vec3 position;
    double opening = 0.0;
    double influenceRadiusM = 1.0;
};

struct HeaterSpec {
    std::string name;
    Vec3 position;
    double powerW = 1000.0;
    double influenceRadiusM = 1.0;
};

struct HumidifierSpec {
    std::string name;
    Vec3 position;
    std::string mode = "off";
    double influenceRadiusM = 1.0;
};

struct TerminalViewSpec {
    bool enabled = false;
    std::string field = "temperature";
    int layerZ = 0;
    double frameIntervalSeconds = 600.0;
    int sleepMs = 120;
    bool useColors = true;
    bool clearScreen = true;
    bool loopPlayback = false;
    bool showDevices = true;
    bool projectDevicesToLayer = true;
    bool autoScale = true;
    double fixedMinValue = 0.0;
    double fixedMaxValue = 40.0;
};

struct OutputSpec {
    std::string directory = "outputs";
    bool writeCsv = true;
    bool writeJson = true;
    bool writeReport = true;
    TerminalViewSpec terminalView;
};

struct OptimizerSpec {
    bool enabled = false;
    double candidateStepM = 1.0;
    int maxCandidates = 30;
    int maxLayouts = 500;
    double energyWeight = 0.05;
};

} // namespace greenhouse
