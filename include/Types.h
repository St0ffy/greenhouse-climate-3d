#pragma once

#include <cstddef>
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
    double lightWm2 = 0.0;
};

struct PlantPoint {
    std::string name;
    Vec3 position;
    double targetTemperatureC = 22.0;
    double targetHumidityPercent = 60.0;
    double targetLightWm2 = 350.0;
    double minSurvivalTemperatureC = 5.0;
    double maxSurvivalTemperatureC = 38.0;
    double minSurvivalHumidityPercent = 25.0;
    double maxSurvivalHumidityPercent = 95.0;
    double minSurvivalLightWm2 = 0.0;
    double maxSurvivalLightWm2 = 1200.0;
    double initialHealth = 1.0;
    double initialGrowth = 0.0;
    double sensorHeightM = 0.4;
};

struct VentSpec {
    std::string name;
    Vec3 position;
    double opening = 0.0;
    double influenceRadiusM = 1.0;
    bool enabled = true;
    bool failed = false;
    double powerW = 20.0;
};

struct HeaterSpec {
    std::string name;
    Vec3 position;
    double powerW = 1000.0;
    double influenceRadiusM = 1.0;
    bool enabled = true;
    bool failed = false;
    double maxPowerW = 0.0;
};

struct HumidifierSpec {
    std::string name;
    Vec3 position;
    std::string mode = "off";
    double influenceRadiusM = 1.0;
    bool enabled = true;
    bool failed = false;
    double level = 1.0;
    double powerW = 150.0;
};

struct TerminalViewSpec {
    bool enabled = false;
    bool interactive = false;
    std::string field = "temperature";
    int layerZ = 0;
    int displayStrideX = 1;
    int displayStrideY = 1;
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

struct PlantPhysicsSpec {
    double humidityUptakePercentPerHour = 0.35;
    double maxHumidityUptakePercentPerStep = 0.25;
};

struct ClimateControlSpec {
    bool enabled = false;
    bool mlEnabled = false;
    bool mlMemoryEnabled = true;
    bool mlMemoryLogEnabled = true;
    std::string strategy = "proportional";
    std::string mlMemoryPath = "outputs/ml_policy.json";
    int mlMemorySaveEverySteps = 50;
    double energyWeight = 0.08;
    double learningRate = 0.25;
    double explorationRate = 0.10;
    double comfortWeight = 1.0;
    double maxHeaterLevelChange = 0.25;
    double temperatureToleranceC = 0.5;
    double humidityTolerancePercent = 3.0;
};

struct PlantSensorReading {
    std::string plantName;
    GridIndex plantCell;
    GridIndex sensorCell;
    std::size_t sensorLinearIndex = 0;
    double temperatureC = 0.0;
    double humidityPercent = 0.0;
    double lightWm2 = 0.0;
};

struct PlantState {
    std::string name;
    double health = 1.0;
    double growth = 0.0;
    double ageSeconds = 0.0;
    double comfort = 0.0;
    bool alive = true;
};

struct PlantGrowthStats {
    double averageHealth = 0.0;
    double minHealth = 0.0;
    double averageGrowth = 0.0;
    double averageComfort = 0.0;
    int aliveCount = 0;
};

struct ControlStepSummary {
    bool enabled = false;
    bool mlEnabled = false;
    double reward = 0.0;
    double activeHeaterPowerW = 0.0;
    double averageVentOpening = 0.0;
    int activeHumidifierCount = 0;
    int failedDeviceCount = 0;
};

} // namespace greenhouse
