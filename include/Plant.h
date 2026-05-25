#pragma once

#include "Devices.h"
#include "Physics.h"
#include "Types.h"

#include <vector>

namespace greenhouse {

struct PlantGrowthStepResult {
    std::vector<PlantState> plants;
    PlantGrowthStats stats;
};

std::vector<PlantState> makeInitialPlantStates(
    const std::vector<MappedPlantPoint>& plants
);

std::vector<PlantSensorReading> readPlantSensors(
    const std::vector<CellState>& cells,
    const std::vector<MappedPlantPoint>& plants
);

double plantComfortScore(
    const PlantPoint& plant,
    const PlantSensorReading& sensor
);

PlantGrowthStats summarizePlantStates(
    const std::vector<PlantState>& plants
);

PlantGrowthStepResult advancePlantGrowth(
    const std::vector<PlantState>& current,
    const std::vector<MappedPlantPoint>& plants,
    const std::vector<PlantSensorReading>& sensors,
    double timeStepSeconds
);

} // namespace greenhouse
