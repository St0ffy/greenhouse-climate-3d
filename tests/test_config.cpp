#include "Config.h"

#include <cassert>
#include <cmath>

namespace {

bool almostEqual(double left, double right) {
    return std::fabs(left - right) < 1e-9;
}

} // namespace

int main() {
    const greenhouse::SimulationConfig config =
        greenhouse::loadConfig("configs/default_config.json");

    assert(config.mode == "simulate");
    assert(almostEqual(config.greenhouseSize.length, 48.0));
    assert(almostEqual(config.greenhouseSize.width, 8.0));
    assert(almostEqual(config.greenhouseSize.height, 3.0));
    assert(config.gridSize.nx == 48);
    assert(config.gridSize.ny == 8);
    assert(config.gridSize.nz == 4);
    assert(almostEqual(config.durationSeconds, 604800.0));
    assert(almostEqual(config.timeStepSeconds, 60.0));

    assert(almostEqual(config.weather.outsideTemperatureC, 5.0));
    assert(almostEqual(config.weather.outsideHumidityPercent, 45.0));
    assert(almostEqual(config.weather.solarRadiationWm2, 320.0));
    assert(config.weather.weatherFile == "data/week_weather.csv");

    assert(config.material.name == "polycarbonate");
    assert(almostEqual(config.material.heatLossCoefficient, 0.38));
    assert(almostEqual(config.material.solarTransmission, 0.75));

    assert(config.vents.size() == 16);
    assert(config.heaters.size() == 8);
    assert(config.humidifiers.size() == 6);
    assert(config.plants.size() == 21);
    assert(almostEqual(config.plants[0].targetHumidityPercent, 62.0));
    assert(almostEqual(config.plants[0].targetLightWm2, 300.0));
    assert(almostEqual(config.plants[0].sensorHeightM, 0.4));
    assert(almostEqual(config.heaters[0].maxPowerW, 1800.0));
    assert(config.heaters[0].enabled);
    assert(!config.heaters[0].failed);
    assert(config.humidifiers[0].enabled);
    assert(almostEqual(config.humidifiers[0].level, 1.0));
    assert(config.output.directory == "outputs");
    assert(!config.output.writeCsv);
    assert(config.output.writeJson);
    assert(config.output.writeReport);
    assert(config.output.terminalView.enabled);
    assert(config.output.terminalView.interactive);
    assert(config.output.terminalView.field == "temperature");
    assert(config.output.terminalView.layerZ == 0);
    assert(config.output.terminalView.displayStrideX == 2);
    assert(config.output.terminalView.displayStrideY == 1);
    assert(almostEqual(config.output.terminalView.frameIntervalSeconds, 3600.0));
    assert(config.output.terminalView.sleepMs == 80);
    assert(config.output.terminalView.useColors);
    assert(config.output.terminalView.clearScreen);
    assert(!config.output.terminalView.loopPlayback);
    assert(config.output.terminalView.showDevices);
    assert(config.output.terminalView.projectDevicesToLayer);
    assert(!config.output.terminalView.autoScale);
    assert(almostEqual(config.output.terminalView.fixedMinValue, 0.0));
    assert(almostEqual(config.output.terminalView.fixedMaxValue, 40.0));
    assert(!config.optimizer.enabled);
    assert(almostEqual(config.optimizer.candidateStepM, 2.0));
    assert(config.optimizer.maxCandidates == 40);
    assert(config.optimizer.maxLayouts == 500);
    assert(almostEqual(config.optimizer.energyWeight, 0.05));
    assert(almostEqual(config.plantPhysics.humidityUptakePercentPerHour, 0.45));
    assert(almostEqual(config.plantPhysics.maxHumidityUptakePercentPerStep, 0.18));
    assert(config.control.enabled);
    assert(config.control.mlEnabled);
    assert(config.control.mlMemoryEnabled);
    assert(config.control.mlMemoryLogEnabled);
    assert(config.control.strategy == "proportional");
    assert(config.control.mlMemoryPath == "outputs/ml_policy.json");
    assert(config.control.mlMemorySaveEverySteps == 50);
    assert(almostEqual(config.control.energyWeight, 0.08));
    assert(almostEqual(config.control.learningRate, 0.25));
    assert(almostEqual(config.control.explorationRate, 0.12));
    assert(almostEqual(config.control.temperatureToleranceC, 0.5));
    assert(almostEqual(config.control.humidityTolerancePercent, 3.0));

    return 0;
}
