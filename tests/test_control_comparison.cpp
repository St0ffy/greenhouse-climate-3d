#include "Comparison.h"
#include "Devices.h"
#include "Geometry.h"
#include "Material.h"
#include "Report.h"
#include "Weather.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <string>

int main() {
    greenhouse::SimulationConfig config;
    config.greenhouseSize = {3.0, 2.0, 1.0};
    config.gridSize = {3, 2, 1};
    config.durationSeconds = 180.0;
    config.timeStepSeconds = 60.0;
    config.initialTemperatureC = 18.0;
    config.initialHumidityPercent = 52.0;
    config.weather = {5.0, 70.0, 120.0, ""};
    config.material = {"polycarbonate", 0.35, 0.75};
    config.plants = {
        {"plant", {0.5, 0.5, 0.5}, 22.0, 60.0, 300.0}
    };
    config.heaters = {
        {"heater", {0.5, 0.5, 0.5}, 500.0, 1.0, true, false, 1000.0}
    };
    config.vents = {
        {"vent", {2.5, 1.5, 0.5}, 0.2, 1.0}
    };
    config.humidifiers = {
        {"humidifier", {0.5, 0.5, 0.5}, "medium", 1.0}
    };
    config.output.directory = "outputs/test_compare";
    config.output.terminalView.enabled = true;
    config.output.terminalView.interactive = true;
    config.control.mlMemoryPath = "outputs/test_compare/ml_policy.json";

    const std::filesystem::path policyPath(config.control.mlMemoryPath);
    std::filesystem::remove(policyPath);

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

    const greenhouse::ControlComparisonResult comparison =
        greenhouse::runControlComparison(config, grid, weather, material, devices);

    assert(comparison.onOff.label == "on_off");
    assert(comparison.ml.label == "ml");
    assert(comparison.onOff.result.config.mode == "compare:on_off");
    assert(comparison.ml.result.config.mode == "compare:ml");
    assert(comparison.onOff.result.config.control.enabled);
    assert(comparison.onOff.result.config.control.strategy == "on_off");
    assert(!comparison.onOff.result.config.control.mlEnabled);
    assert(comparison.ml.result.config.control.enabled);
    assert(comparison.ml.result.config.control.strategy == "proportional");
    assert(comparison.ml.result.config.control.mlEnabled);
    assert(comparison.ml.result.config.control.mlMemoryEnabled);
    assert(!comparison.ml.result.config.control.mlMemoryLogEnabled);
    assert(!comparison.ml.result.config.output.terminalView.enabled);
    assert(!comparison.ml.result.config.output.terminalView.interactive);
    assert(std::filesystem::exists(policyPath));
    assert(
        comparison.onOff.result.config.output.directory.find("on_off")
        != std::string::npos
    );
    assert(
        comparison.ml.result.config.output.directory.find("ml")
        != std::string::npos
    );
    assert(!comparison.onOff.result.frames.empty());
    assert(!comparison.ml.result.frames.empty());
    assert(!comparison.recommendation.empty());
    assert(std::fabs(comparison.onOffQuality - comparison.mlQuality) > 1e-9);

    const std::string terminalSummary =
        greenhouse::buildControlComparisonTerminalSummary(comparison);
    assert(terminalSummary.find("ON_OFF") != std::string::npos);
    assert(terminalSummary.find("ML") != std::string::npos);
    assert(terminalSummary.find("BEST") != std::string::npos);
    assert(terminalSummary.find("Metric") != std::string::npos);
    assert(terminalSummary.find("Verdict") == std::string::npos);

    std::filesystem::remove(policyPath);

    return 0;
}
