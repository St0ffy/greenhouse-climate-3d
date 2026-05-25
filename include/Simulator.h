#pragma once

#include "Config.h"
#include "Controller.h"
#include "Devices.h"
#include "Geometry.h"
#include "Material.h"
#include "Plant.h"
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
    PlantGrowthStats plantGrowth;
    ControlStepSummary control;
    double cumulativeHeaterEnergyKWh = 0.0;
    double cumulativeDeviceEnergyKWh = 0.0;
    std::vector<CellState> cells;
    std::vector<PlantState> plants;
    std::vector<PlantSensorReading> sensorReadings;
    MappedDeviceSet devices;
};

struct SimulationResult {
    SimulationConfig config;
    MaterialProperties material;
    std::vector<SimulationFrame> frames;
    double durationSeconds = 0.0;
    double timeStepSeconds = 0.0;
    double totalHeaterEnergyKWh = 0.0;
    double totalDeviceEnergyKWh = 0.0;
};

class SimulationStepper {
public:
    SimulationStepper(
        const SimulationConfig& config,
        const Grid3D& grid,
        const WeatherTimeline& weather,
        const MaterialProperties& material,
        const MappedDeviceSet& devices,
        const ClimatePhysicsSettings& settings = ClimatePhysicsSettings{}
    );

    bool finished() const;
    void step();
    bool toggleDeviceFailureAt(const GridIndex& cell);

    const SimulationResult& result() const;
    const SimulationFrame& currentFrame() const;
    const MappedDeviceSet& devices() const;
    double currentTimeSeconds() const;

private:
    SimulationConfig config_;
    const Grid3D& grid_;
    const WeatherTimeline& weather_;
    MaterialProperties material_;
    ClimatePhysicsSettings settings_;
    AdaptiveClimateController controller_;
    MappedDeviceSet devices_;
    std::vector<CellState> cells_;
    std::vector<PlantState> plantStates_;
    SimulationResult result_;
    double currentTime_ = 0.0;
    double cumulativeHeaterEnergy_ = 0.0;
    double cumulativeDeviceEnergy_ = 0.0;
    int mlMemoryStepsSinceSave_ = 0;
    bool mlMemoryActive_ = false;

    void updateInitialLight();
    void maybeSaveMlPolicy(bool force);
    void appendFrame(
        const WeatherCondition& weather,
        const ClimateStepSummary& climate,
        const ControlStepSummary& control
    );
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
