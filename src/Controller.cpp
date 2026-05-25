#include "Controller.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>

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

std::string currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream output;
    output << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

bool readDoubleField(const std::string& text, const std::string& key, double& value) {
    const std::regex pattern(
        "\"" + key + "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)"
    );
    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        return false;
    }

    value = std::stod(match[1].str());
    return true;
}

bool readIntField(const std::string& text, const std::string& key, int& value) {
    double numeric = 0.0;
    if (!readDoubleField(text, key, numeric)) {
        return false;
    }

    value = static_cast<int>(numeric);
    return true;
}

bool readDoubleArray(
    const std::string& text,
    const std::string& key,
    std::vector<double>& values
) {
    const std::string quotedKey = "\"" + key + "\"";
    const std::size_t keyPosition = text.find(quotedKey);
    if (keyPosition == std::string::npos) {
        return false;
    }

    const std::size_t arrayStart = text.find('[', keyPosition + quotedKey.size());
    if (arrayStart == std::string::npos) {
        return false;
    }

    const std::size_t arrayEnd = text.find(']', arrayStart + 1);
    if (arrayEnd == std::string::npos) {
        return false;
    }

    values.clear();
    const std::string arrayText =
        text.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    const std::regex numberPattern(
        "-?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?"
    );
    for (std::sregex_iterator it(
             arrayText.begin(),
             arrayText.end(),
             numberPattern
         );
         it != std::sregex_iterator{};
         ++it) {
        values.push_back(std::stod((*it)[0].str()));
    }

    return true;
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

bool AdaptiveClimateController::loadPolicy(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        std::cout << "[ML] No existing policy found, starting fresh\n";
        return false;
    }

    const std::string text{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };

    int version = 0;
    int actionCount = 0;
    std::vector<double> values;
    if (!readIntField(text, "version", version)
        || version != 1
        || !readIntField(text, "action_count", actionCount)
        || actionCount != static_cast<int>(actionValues_.size())
        || !readDoubleArray(text, "action_values", values)
        || values.size() != actionValues_.size()) {
        std::cout << "[ML] Invalid policy file, starting fresh\n";
        std::fill(actionValues_.begin(), actionValues_.end(), 0.0);
        actionValues_[13] = 1e-6;
        lastReward_ = 0.0;
        return false;
    }

    double lastReward = 0.0;
    readDoubleField(text, "last_reward", lastReward);
    actionValues_ = values;
    lastReward_ = lastReward;
    std::cout << "[ML] Loaded policy from " << path << "\n";
    return true;
}

bool AdaptiveClimateController::savePolicy(const std::string& path) const {
    const std::filesystem::path policyPath(path);
    const std::filesystem::path parent = policyPath.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream output(path);
    if (!output) {
        std::cout << "[ML] Could not save policy to " << path << "\n";
        return false;
    }

    output << std::setprecision(12);
    output << "{\n";
    output << "  \"version\": 1,\n";
    output << "  \"action_count\": " << actionValues_.size() << ",\n";
    output << "  \"action_values\": [\n";
    for (std::size_t i = 0; i < actionValues_.size(); ++i) {
        output << "    " << actionValues_[i];
        if (i + 1 < actionValues_.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "  ],\n";
    output << "  \"last_reward\": " << lastReward_ << ",\n";
    output << "  \"metadata\": {\n";
    output << "    \"description\": "
           << "\"Persistent ML policy for greenhouse adaptive controller\",\n";
    output << "    \"saved_at\": \"" << currentTimestamp() << "\",\n";
    output << "    \"learning_rate\": " << spec_.learningRate << ",\n";
    output << "    \"exploration_rate\": " << spec_.explorationRate << "\n";
    output << "  }\n";
    output << "}\n";

    std::cout << "[ML] Saved policy to " << path << "\n";
    return true;
}

} // namespace greenhouse
