#include "Devices.h"
#include "Plant.h"

#include <cassert>
#include <vector>

int main() {
    const greenhouse::Grid3D grid({1.0, 1.0, 1.0}, {1, 1, 1});
    const std::vector<greenhouse::PlantPoint> plants = {
        {
            "plant",
            {0.5, 0.5, 0.2},
            22.0,
            60.0,
            300.0,
            8.0,
            35.0,
            30.0,
            90.0,
            20.0,
            1000.0,
            1.0,
            0.0,
            0.4
        }
    };
    const greenhouse::MappedDeviceSet devices =
        greenhouse::mapDeviceSetToGrid(plants, {}, {}, {}, grid);

    std::vector<greenhouse::PlantState> states =
        greenhouse::makeInitialPlantStates(devices.plants);

    {
        const std::vector<greenhouse::CellState> cells = {
            {22.0, 60.0, 300.0}
        };
        const std::vector<greenhouse::PlantSensorReading> sensors =
            greenhouse::readPlantSensors(cells, devices.plants);
        const greenhouse::PlantGrowthStepResult growth =
            greenhouse::advancePlantGrowth(states, devices.plants, sensors, 3600.0);

        assert(growth.plants.front().growth > states.front().growth);
        assert(growth.plants.front().health >= states.front().health);
        assert(growth.stats.aliveCount == 1);
        states = growth.plants;
    }

    {
        const std::vector<greenhouse::CellState> cells = {
            {2.0, 10.0, 0.0}
        };
        const std::vector<greenhouse::PlantSensorReading> sensors =
            greenhouse::readPlantSensors(cells, devices.plants);
        const greenhouse::PlantGrowthStepResult growth =
            greenhouse::advancePlantGrowth(states, devices.plants, sensors, 3600.0);

        assert(growth.plants.front().health < states.front().health);
        assert(growth.stats.averageComfort < 0.5);
    }

    return 0;
}
