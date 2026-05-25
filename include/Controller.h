#pragma once

#include "Config.h"
#include "Devices.h"
#include "Plant.h"
#include "Types.h"

#include <random>
#include <vector>

namespace greenhouse {

class AdaptiveClimateController {
public:
    explicit AdaptiveClimateController(const ClimateControlSpec& spec);

    ControlStepSummary apply(
        MappedDeviceSet& devices,
        const std::vector<MappedPlantPoint>& plants,
        const std::vector<PlantSensorReading>& sensors,
        const std::vector<PlantState>& plantStates
    );

    void learn(
        const PlantGrowthStats& plantStats,
        double stepEnergyKWh
    );

    double lastReward() const;
    bool loadPolicy(const std::string& path);
    bool savePolicy(const std::string& path) const;

private:
    ClimateControlSpec spec_;
    std::vector<double> actionValues_;
    int lastActionIndex_ = -1;
    double lastReward_ = 0.0;
    std::mt19937 rng_;

    int chooseAction();
};

} // namespace greenhouse
