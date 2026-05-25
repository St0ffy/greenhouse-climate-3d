#include "Controller.h"

#include <cassert>
#include <vector>

int main() {
    const greenhouse::Grid3D grid({2.0, 1.0, 1.0}, {2, 1, 1});
    const std::vector<greenhouse::PlantPoint> plants = {
        {"plant", {0.5, 0.5, 0.5}, 22.0, 60.0, 300.0}
    };
    const std::vector<greenhouse::HeaterSpec> heaters = {
        {"heater_failed", {0.5, 0.5, 0.5}, 500.0, 1.0, true, true, 1000.0},
        {"heater_ok", {1.5, 0.5, 0.5}, 500.0, 1.0, true, false, 1000.0}
    };
    const std::vector<greenhouse::HumidifierSpec> humidifiers = {
        {"humidifier", {0.5, 0.5, 0.5}, "off", 1.0}
    };

    greenhouse::MappedDeviceSet devices =
        greenhouse::mapDeviceSetToGrid(plants, {}, heaters, humidifiers, grid);
    const std::vector<greenhouse::PlantState> states =
        greenhouse::makeInitialPlantStates(devices.plants);
    const std::vector<greenhouse::PlantSensorReading> sensors = {
        {"plant", {0, 0, 0}, {0, 0, 0}, 0, 12.0, 30.0, 250.0}
    };

    greenhouse::ClimateControlSpec spec;
    spec.enabled = true;
    spec.mlEnabled = false;
    greenhouse::AdaptiveClimateController controller(spec);
    const greenhouse::ControlStepSummary summary =
        controller.apply(devices, devices.plants, sensors, states);

    assert(summary.enabled);
    assert(summary.failedDeviceCount == 1);
    assert(devices.heaters[0].spec.powerW == 0.0);
    assert(devices.heaters[1].spec.powerW > 0.0);
    assert(devices.humidifiers.front().spec.mode != "off");

    greenhouse::PlantGrowthStats stats;
    stats.averageComfort = 0.8;
    stats.averageHealth = 1.0;
    controller.learn(stats, 0.01);
    assert(controller.lastReward() > 0.0);

    return 0;
}
