#include "Config.h"
#include "Geometry.h"
#include "Material.h"
#include "Optimizer.h"
#include "Weather.h"

#include <cassert>

int main() {
    greenhouse::SimulationConfig config;
    config.mode = "optimize";
    config.greenhouseSize = {3.0, 1.0, 1.0};
    config.gridSize = {3, 1, 1};
    config.durationSeconds = 180.0;
    config.timeStepSeconds = 60.0;
    config.initialTemperatureC = 15.0;
    config.initialHumidityPercent = 50.0;
    config.weather = {5.0, 70.0, 0.0, ""};
    config.material = {"polycarbonate", 0.0, 0.0};
    config.plants = {{"plant", {0.5, 0.5, 0.5}, 22.0}};
    config.heaters = {{"heater", {2.5, 0.5, 0.5}, 1500.0, 0.1}};
    config.optimizer = {true, 1.0, 3, 10, 0.0};
    config.output = {"outputs/test_optimizer", false, false, false};

    const greenhouse::Grid3D grid(config.greenhouseSize, config.gridSize);
    const greenhouse::MaterialProperties material =
        greenhouse::makeMaterial(config.material);
    const greenhouse::WeatherTimeline weather(config.weather);
    const greenhouse::OptimizationResult result =
        greenhouse::optimizeHeaterPlacement(config, grid, weather, material);

    assert(result.testedLayouts > 0);
    assert(result.candidatePositions.size() == 3);
    assert(result.bestScore.quality <= result.baselineScore.quality);
    assert(result.bestScore.averagePlantTemperatureErrorC
        <= result.baselineScore.averagePlantTemperatureErrorC);
    assert(!result.bestDevices.heaters.empty());

    return 0;
}
