#pragma once

#include "Geometry.h"
#include "Types.h"

#include <cstddef>
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

} // namespace greenhouse
