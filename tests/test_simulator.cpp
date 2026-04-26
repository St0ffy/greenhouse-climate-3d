#include "Config.h"
#include "Devices.h"
#include "Geometry.h"
#include "Material.h"
#include "Simulator.h"
#include "Weather.h"

#include <cassert>
#include <cmath>

int main() {
    greenhouse::SimulationConfig config;
    config.greenhouseSize = {2.0, 2.0, 1.0};
    config.gridSize = {2, 2, 1};
    config.durationSeconds = 120.0;
    config.timeStepSeconds = 60.0;
    config.initialTemperatureC = 18.0;
    config.initialHumidityPercent = 50.0;
    config.weather = {4.0, 80.0, 100.0, ""};
    config.material = {"polycarbonate", 0.35, 0.75};
    config.plants = {{"plant", {0.5, 0.5, 0.5}, 22.0}};
    config.heaters = {{"heater", {0.5, 0.5, 0.5}, 500.0, 1.0}};
    config.vents = {{"vent", {1.5, 1.5, 0.5}, 0.2, 1.0}};
    config.humidifiers = {{"humidifier", {0.5, 0.5, 0.5}, "medium", 1.0}};

    const greenhouse::Grid3D grid(config.greenhouseSize, config.gridSize);
    const greenhouse::MappedDeviceSet devices =
        greenhouse::mapDeviceSetToGrid(
            config.plants,
            config.vents,
            config.heaters,
            config.humidifiers,
            grid
        );
    const greenhouse::MaterialProperties material =
        greenhouse::makeMaterial(config.material);
    const greenhouse::WeatherTimeline weather(config.weather);
    const greenhouse::SimulationResult result =
        greenhouse::runSimulation(config, grid, weather, material, devices);

    assert(result.frames.size() == 3);
    assert(result.frames.front().timeSeconds == 0.0);
    assert(result.frames.back().timeSeconds == 120.0);
    assert(result.frames.back().cells.size() == grid.cellCount());
    assert(result.totalHeaterEnergyKWh > 0.0);
    assert(result.frames.back().plantTemperature.averageAbsoluteErrorC >= 0.0);
    assert(result.frames.back().plantHumidity.averageHumidityPercent >= 0.0);
    assert(result.frames.back().plantHumidity.averageHumidityPercent <= 100.0);

    return 0;
}
