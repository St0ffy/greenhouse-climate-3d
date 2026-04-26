#include "Simulator.h"

#include <algorithm>
#include <stdexcept>

namespace greenhouse {

namespace {

void validateSimulationConfig(const SimulationConfig& config) {
    if (config.durationSeconds < 0.0) {
        throw std::invalid_argument("simulation duration cannot be negative");
    }

    if (config.timeStepSeconds <= 0.0) {
        throw std::invalid_argument("simulation time step must be positive");
    }
}

ClimateStepSummary makeInitialSummary(const std::vector<CellState>& cells) {
    ClimateStepSummary summary;
    summary.temperatureStep.temperature = summarizeTemperature(cells);
    summary.humidity = summarizeHumidity(cells);
    return summary;
}

SimulationFrame makeFrame(
    double timeSeconds,
    const WeatherCondition& weather,
    const ClimateStepSummary& climate,
    const std::vector<CellState>& cells,
    const std::vector<MappedPlantPoint>& plants,
    double cumulativeEnergyKWh
) {
    return {
        timeSeconds,
        weather,
        climate,
        summarizePlantTemperatures(cells, plants),
        summarizePlantHumidity(cells, plants),
        cumulativeEnergyKWh,
        cells
    };
}

} // namespace

SimulationResult runSimulation(
    const SimulationConfig& config,
    const Grid3D& grid,
    const WeatherTimeline& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices,
    const ClimatePhysicsSettings& settings
) {
    validateSimulationConfig(config);

    ClimatePhysicsSettings activeSettings = settings;
    activeSettings.humidity.humidityEnabled = config.humidityEnabled;

    SimulationResult result;
    result.config = config;
    result.material = material;
    result.durationSeconds = config.durationSeconds;
    result.timeStepSeconds = config.timeStepSeconds;

    std::vector<CellState> cells =
        makeInitialCells(
            grid,
            config.initialTemperatureC,
            config.initialHumidityPercent
        );

    double currentTime = 0.0;
    double cumulativeEnergy = 0.0;

    result.frames.push_back(
        makeFrame(
            currentTime,
            weather.at(currentTime),
            makeInitialSummary(cells),
            cells,
            devices.plants,
            cumulativeEnergy
        )
    );

    while (currentTime < config.durationSeconds) {
        const double stepSeconds =
            std::min(config.timeStepSeconds, config.durationSeconds - currentTime);
        if (stepSeconds <= 0.0) {
            break;
        }

        const WeatherCondition stepWeather = weather.at(currentTime);
        const ClimateStepResult step =
            advanceClimate(
                cells,
                grid,
                stepWeather,
                material,
                devices,
                stepSeconds,
                activeSettings
            );

        cells = step.cells;
        currentTime += stepSeconds;
        cumulativeEnergy += step.summary.temperatureStep.heaterEnergyKWh;

        result.frames.push_back(
            makeFrame(
                currentTime,
                weather.at(currentTime),
                step.summary,
                cells,
                devices.plants,
                cumulativeEnergy
            )
        );
    }

    result.totalHeaterEnergyKWh = cumulativeEnergy;
    return result;
}

} // namespace greenhouse
