#include "Controller.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
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

    const std::vector<greenhouse::PlantSensorReading> satisfiedSensors = {
        {"plant", {0, 0, 0}, {0, 0, 0}, 0, 22.0, 61.0, 300.0}
    };
    const greenhouse::ControlStepSummary satisfiedSummary =
        controller.apply(devices, devices.plants, satisfiedSensors, states);
    assert(satisfiedSummary.enabled);
    assert(devices.humidifiers.front().spec.mode == "off");

    {
        const std::vector<greenhouse::VentSpec> vents = {
            {"vent", {1.5, 0.5, 0.5}, 0.0, 1.0}
        };
        greenhouse::MappedDeviceSet onOffDevices =
            greenhouse::mapDeviceSetToGrid(plants, vents, heaters, humidifiers, grid);

        greenhouse::ClimateControlSpec onOffSpec;
        onOffSpec.enabled = true;
        onOffSpec.strategy = "on_off";
        onOffSpec.temperatureToleranceC = 0.5;
        onOffSpec.humidityTolerancePercent = 3.0;
        greenhouse::AdaptiveClimateController onOffController(onOffSpec);

        const std::vector<greenhouse::PlantSensorReading> coldDrySensors = {
            {"plant", {0, 0, 0}, {0, 0, 0}, 0, 20.0, 50.0, 250.0}
        };
        onOffController.apply(onOffDevices, onOffDevices.plants, coldDrySensors, states);
        assert(onOffDevices.heaters[0].spec.powerW == 0.0);
        assert(onOffDevices.heaters[1].spec.powerW == 1000.0);
        assert(onOffDevices.humidifiers.front().spec.mode == "high");
        assert(onOffDevices.vents.front().spec.opening == 0.0);

        const std::vector<greenhouse::PlantSensorReading> targetSensors = {
            {"plant", {0, 0, 0}, {0, 0, 0}, 0, 21.6, 58.0, 300.0}
        };
        onOffController.apply(onOffDevices, onOffDevices.plants, targetSensors, states);
        assert(onOffDevices.heaters[1].spec.powerW == 0.0);
        assert(onOffDevices.humidifiers.front().spec.mode == "off");
        assert(onOffDevices.vents.front().spec.opening == 0.0);

        const std::vector<greenhouse::PlantSensorReading> hotWetSensors = {
            {"plant", {0, 0, 0}, {0, 0, 0}, 0, 24.0, 66.0, 300.0}
        };
        onOffController.apply(onOffDevices, onOffDevices.plants, hotWetSensors, states);
        assert(onOffDevices.heaters[1].spec.powerW == 0.0);
        assert(onOffDevices.humidifiers.front().spec.mode == "off");
        assert(onOffDevices.vents.front().spec.opening == 1.0);
    }

    greenhouse::PlantGrowthStats stats;
    stats.averageComfort = 0.8;
    stats.averageHealth = 1.0;
    controller.learn(stats, 0.01);
    assert(controller.lastReward() > 0.0);

    const std::filesystem::path policyPath =
        std::filesystem::path("outputs") / "test_ml_policy.json";
    std::filesystem::remove(policyPath);
    assert(!controller.loadPolicy(policyPath.string()));
    assert(controller.savePolicy(policyPath.string()));
    assert(std::filesystem::exists(policyPath));

    greenhouse::ClimateControlSpec mlSpec;
    mlSpec.enabled = true;
    mlSpec.mlEnabled = true;
    greenhouse::AdaptiveClimateController loadedController(mlSpec);
    assert(loadedController.loadPolicy(policyPath.string()));
    assert(std::fabs(loadedController.lastReward() - controller.lastReward()) < 1e-9);

    {
        std::ofstream invalidPolicy(policyPath);
        invalidPolicy << "{ \"version\": 1, \"action_count\": 1, "
                      << "\"action_values\": [0.0] }";
    }
    assert(!loadedController.loadPolicy(policyPath.string()));
    std::filesystem::remove(policyPath);

    return 0;
}
