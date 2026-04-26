#include "Devices.h"
#include "Exporter.h"
#include "Geometry.h"
#include "Material.h"
#include "Report.h"
#include "Simulator.h"
#include "Weather.h"

#include <cassert>
#include <filesystem>
#include <string>

int main() {
    greenhouse::SimulationConfig config;
    config.greenhouseSize = {1.0, 1.0, 1.0};
    config.gridSize = {1, 1, 1};
    config.durationSeconds = 60.0;
    config.timeStepSeconds = 60.0;
    config.initialTemperatureC = 20.0;
    config.initialHumidityPercent = 55.0;
    config.weather = {5.0, 80.0, 0.0, ""};
    config.material = {"polycarbonate", 0.35, 0.75};
    config.plants = {{"plant", {0.5, 0.5, 0.5}, 22.0}};
    config.output = {"outputs/test_day6", true, true, true};

    const greenhouse::Grid3D grid(config.greenhouseSize, config.gridSize);
    const greenhouse::MappedDeviceSet devices =
        greenhouse::mapDeviceSetToGrid(
            config.plants,
            {},
            {},
            {},
            grid
        );
    const greenhouse::MaterialProperties material =
        greenhouse::makeMaterial(config.material);
    const greenhouse::WeatherTimeline weather(config.weather);
    const greenhouse::SimulationResult result =
        greenhouse::runSimulation(config, grid, weather, material, devices);

    const greenhouse::ExportedFiles exported =
        greenhouse::exportSimulationResult(result, grid, config.output);
    const std::string reportText =
        greenhouse::buildSimulationReport(result, grid, devices);
    const std::string reportPath =
        greenhouse::writeReportFile(reportText, config.output);

    assert(!exported.cellHistoryCsvPath.empty());
    assert(!exported.metricsCsvPath.empty());
    assert(!exported.summaryJsonPath.empty());
    assert(!reportPath.empty());
    assert(std::filesystem::exists(exported.cellHistoryCsvPath));
    assert(std::filesystem::exists(exported.metricsCsvPath));
    assert(std::filesystem::exists(exported.summaryJsonPath));
    assert(std::filesystem::exists(reportPath));
    assert(reportText.find("Greenhouse Climate 3D Report") != std::string::npos);

    return 0;
}
