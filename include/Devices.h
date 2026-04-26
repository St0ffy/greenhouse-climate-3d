#pragma once

#include "Geometry.h"
#include "Types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace greenhouse {

struct DeviceInfluenceCell {
    GridIndex index;
    std::size_t linearIndex = 0;
    Vec3 center;
    double distanceM = 0.0;
    double weight = 1.0;
};

struct MappedPlantPoint {
    PlantPoint plant;
    GridIndex cell;
    std::size_t linearIndex = 0;
    Vec3 cellCenter;
    double distanceToCenterM = 0.0;
};

struct MappedVent {
    VentSpec spec;
    GridIndex anchorCell;
    std::size_t anchorLinearIndex = 0;
    std::vector<DeviceInfluenceCell> influenceCells;
};

struct MappedHeater {
    HeaterSpec spec;
    GridIndex anchorCell;
    std::size_t anchorLinearIndex = 0;
    std::vector<DeviceInfluenceCell> influenceCells;
};

struct MappedHumidifier {
    HumidifierSpec spec;
    GridIndex anchorCell;
    std::size_t anchorLinearIndex = 0;
    std::vector<DeviceInfluenceCell> influenceCells;
};

struct MappedDeviceSet {
    std::vector<MappedPlantPoint> plants;
    std::vector<MappedVent> vents;
    std::vector<MappedHeater> heaters;
    std::vector<MappedHumidifier> humidifiers;
    double totalHeaterPowerW = 0.0;
    double averageVentOpening = 0.0;
};

bool isValidHumidifierMode(const std::string& mode);
double humidifierModeMultiplier(const std::string& mode);

void validateDeviceSpecs(
    const std::vector<PlantPoint>& plants,
    const std::vector<VentSpec>& vents,
    const std::vector<HeaterSpec>& heaters,
    const std::vector<HumidifierSpec>& humidifiers
);

std::vector<MappedPlantPoint> mapPlantsToGrid(
    const std::vector<PlantPoint>& plants,
    const Grid3D& grid
);

std::vector<MappedVent> mapVentsToGrid(
    const std::vector<VentSpec>& vents,
    const Grid3D& grid
);

std::vector<MappedHeater> mapHeatersToGrid(
    const std::vector<HeaterSpec>& heaters,
    const Grid3D& grid
);

std::vector<MappedHumidifier> mapHumidifiersToGrid(
    const std::vector<HumidifierSpec>& humidifiers,
    const Grid3D& grid
);

MappedDeviceSet mapDeviceSetToGrid(
    const std::vector<PlantPoint>& plants,
    const std::vector<VentSpec>& vents,
    const std::vector<HeaterSpec>& heaters,
    const std::vector<HumidifierSpec>& humidifiers,
    const Grid3D& grid
);

} // namespace greenhouse
