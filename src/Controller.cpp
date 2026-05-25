#include "Controller.h"

#include <algorithm>
#include <cmath>

namespace greenhouse {

namespace {

struct Demand {
    double heater = 0.0;
    double humidifier = 0.0;
    double vent = 0.0;
};

struct ActionOffsets {
    double heater = 0.0;
    double humidifier = 0.0;
    double vent = 0.0;
};

double safeAverage(double total, std::size_t count) {
    return count == 0 ? 0.0 : total / static_cast<double>(count);
}

Demand fallbackDemand(
    const std::vector<MappedPlantPoint>& plants,
    const std::vector<PlantSensorReading>& sensors
) {
    Demand demand;
    if (plants.empty() || sensors.empty()) {
        return demand;
    }

    double coldError = 0.0;
    double hotError = 0.0;
    double dryError = 0.0;
    double wetError = 0.0;
    const std::size_t count = std::min(plants.size(), sensors.size());

    for (std::size_t i = 0; i < count; ++i) {
        const PlantPoint& plant = plants[i].plant;
        const PlantSensorReading& sensor = sensors[i];

        coldError += std::max(0.0, plant.targetTemperatureC - sensor.temperatureC);
        hotError += std::max(0.0, sensor.temperatureC - plant.targetTemperatureC);
        dryError += std::max(0.0, plant.targetHumidityPercent - sensor.humidityPercent);
        wetError += std::max(0.0, sensor.humidityPercent - plant.targetHumidityPercent);
    }

    demand.heater = std::clamp(safeAverage(coldError, count) / 8.0, 0.0, 1.0);
    demand.humidifier = std::clamp(safeAverage(dryError, count) / 25.0, 0.0, 1.0);
    demand.vent = std::clamp(
        safeAverage(hotError, count) / 8.0
            + safeAverage(wetError, count) / 60.0,
        0.0,
        1.0
    );

    return demand;
}

ActionOffsets actionOffsets(int index, double maxChange) {
    const double values[3] = {-maxChange, 0.0, maxChange};
    const int heaterIndex = index % 3;
    const int humidifierIndex = (index / 3) % 3;
    const int ventIndex = (index / 9) % 3;
    return {
        values[heaterIndex],
        values[humidifierIndex],
        values[ventIndex]
    };
}

void setHumidifierMode(MappedHumidifier& humidifier, double demand) {
    if (!humidifier.spec.enabled || humidifier.spec.failed || demand <= 0.02) {
        humidifier.spec.mode = "off";
        humidifier.spec.level = 0.0;
        return;
    }

    humidifier.spec.level = std::clamp(demand, 0.0, 1.0);
    if (demand < 0.34) {
        humidifier.spec.mode = "low";
    } else if (demand < 0.67) {
        humidifier.spec.mode = "medium";
    } else {
        humidifier.spec.mode = "high";
    }
}

int countFailedDevices(const MappedDeviceSet& devices) {
    int count = 0;
    for (const MappedHeater& heater : devices.heaters) {
        if (heater.spec.failed) {
            ++count;
        }
    }
    for (const MappedHumidifier& humidifier : devices.humidifiers) {
        if (humidifier.spec.failed) {
            ++count;
        }
    }
    for (const MappedVent& vent : devices.vents) {
        if (vent.spec.failed) {
            ++count;
        }
    }
    return count;
}

int countActiveHumidifiers(const MappedDeviceSet& devices) {
    int count = 0;
    for (const MappedHumidifier& humidifier : devices.humidifiers) {
        if (humidifier.spec.enabled
            && !humidifier.spec.failed
            && humidifierModeMultiplier(humidifier.spec.mode) > 0.0
            && humidifier.spec.level > 0.0) {
            ++count;
        }
    }
    return count;
}

} // namespace

AdaptiveClimateController::AdaptiveClimateController(const ClimateControlSpec& spec)
    : spec_(spec),
      actionValues_(27, 0.0),
      rng_(42) {
    actionValues_[13] = 1e-6;
}

int AdaptiveClimateController::chooseAction() {
    if (!spec_.mlEnabled) {
        return 13;
    }

    std::uniform_real_distribution<double> probability(0.0, 1.0);
    if (probability(rng_) < std::clamp(spec_.explorationRate, 0.0, 1.0)) {
        std::uniform_int_distribution<int> action(0, 26);
        return action(rng_);
    }

    return static_cast<int>(
        std::distance(
            actionValues_.begin(),
            std::max_element(actionValues_.begin(), actionValues_.end())
        )
    );
}

ControlStepSummary AdaptiveClimateController::apply(
    MappedDeviceSet& devices,
    const std::vector<MappedPlantPoint>& plants,
    const std::vector<PlantSensorReading>& sensors,
    const std::vector<PlantState>& plantStates
) {
    (void)plantStates;

    if (!spec_.enabled) {
        refreshDeviceRuntimeTotals(devices);
        return {
            false,
            false,
            lastReward_,
            devices.totalHeaterPowerW,
            devices.averageVentOpening,
            countActiveHumidifiers(devices),
            countFailedDevices(devices)
        };
    }

    const Demand demand = fallbackDemand(plants, sensors);
    lastActionIndex_ = chooseAction();
    const ActionOffsets offsets =
        spec_.mlEnabled
            ? actionOffsets(lastActionIndex_, std::max(0.0, spec_.maxHeaterLevelChange))
            : ActionOffsets{};

    const double heaterDemand = std::clamp(demand.heater + offsets.heater, 0.0, 1.0);
    const double humidifierDemand =
        std::clamp(demand.humidifier + offsets.humidifier, 0.0, 1.0);
    const double ventDemand = std::clamp(demand.vent + offsets.vent, 0.0, 1.0);

    for (MappedHeater& heater : devices.heaters) {
        if (!heater.spec.enabled || heater.spec.failed) {
            heater.spec.powerW = 0.0;
            continue;
        }
        heater.spec.powerW = effectiveHeaterMaxPowerW(heater.spec) * heaterDemand;
    }

    for (MappedHumidifier& humidifier : devices.humidifiers) {
        setHumidifierMode(humidifier, humidifierDemand);
    }

    for (MappedVent& vent : devices.vents) {
        if (!vent.spec.enabled || vent.spec.failed) {
            vent.spec.opening = 0.0;
            continue;
        }
        vent.spec.opening = ventDemand;
    }

    refreshDeviceRuntimeTotals(devices);
    return {
        true,
        spec_.mlEnabled,
        lastReward_,
        devices.totalHeaterPowerW,
        devices.averageVentOpening,
        countActiveHumidifiers(devices),
        countFailedDevices(devices)
    };
}

void AdaptiveClimateController::learn(
    const PlantGrowthStats& plantStats,
    double stepEnergyKWh
) {
    const double reward =
        spec_.comfortWeight * plantStats.averageComfort
        + 0.5 * plantStats.averageHealth
        - spec_.energyWeight * stepEnergyKWh * 100.0;
    lastReward_ = reward;

    if (!spec_.enabled || !spec_.mlEnabled || lastActionIndex_ < 0) {
        return;
    }

    const double learningRate = std::clamp(spec_.learningRate, 0.0, 1.0);
    actionValues_[lastActionIndex_] +=
        learningRate * (reward - actionValues_[lastActionIndex_]);
}

double AdaptiveClimateController::lastReward() const {
    return lastReward_;
}

} // namespace greenhouse
