#include "Devices.h"

#include <cassert>
#include <stdexcept>
#include <vector>

int main() {
    const greenhouse::Grid3D grid({12.0, 6.0, 3.0}, {12, 6, 4});

    const std::vector<greenhouse::PlantPoint> plants = {
        {"plant", {3.0, 2.0, 0.5}, 22.0}
    };
    const std::vector<greenhouse::VentSpec> vents = {
        {"vent", {2.0, 0.5, 2.8}, 0.5, 2.0}
    };
    const std::vector<greenhouse::HeaterSpec> heaters = {
        {"heater", {3.0, 3.0, 0.2}, 1200.0, 2.5}
    };
    const std::vector<greenhouse::HumidifierSpec> humidifiers = {
        {"humidifier", {6.0, 3.0, 0.5}, "medium", 2.0}
    };

    const greenhouse::MappedDeviceSet mapped =
        greenhouse::mapDeviceSetToGrid(plants, vents, heaters, humidifiers, grid);
    assert(mapped.plants.size() == 1);
    assert(mapped.vents.size() == 1);
    assert(mapped.heaters.size() == 1);
    assert(mapped.humidifiers.size() == 1);
    assert(mapped.totalHeaterPowerW == 1200.0);
    assert(mapped.averageVentOpening == 0.5);
    assert(greenhouse::humidifierModeMultiplier("off") == 0.0);
    assert(greenhouse::humidifierModeMultiplier("medium") == 1.0);

    bool failed = false;
    try {
        greenhouse::validateDeviceSpecs(
            plants,
            {{"bad_vent", {1.0, 1.0, 1.0}, 2.0, 1.0}},
            heaters,
            humidifiers
        );
    } catch (const std::invalid_argument&) {
        failed = true;
    }
    assert(failed);

    return 0;
}
