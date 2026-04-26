#include "Devices.h"

#include <algorithm>

namespace greenhouse {

namespace {

double influenceWeight(double distanceM, double radiusM) {
    if (radiusM <= 0.0) {
        return 1.0;
    }

    return std::clamp(1.0 - distanceM / radiusM, 0.05, 1.0);
}

std::vector<DeviceInfluenceCell> buildInfluenceCells(
    const Grid3D& grid,
    const Vec3& position,
    double radiusM
) {
    const std::vector<GridIndex> gridCells = grid.cellsWithinRadius(position, radiusM);

    std::vector<DeviceInfluenceCell> result;
    result.reserve(gridCells.size());
    for (const GridIndex& cell : gridCells) {
        const Vec3 center = grid.cellCenter(cell);
        const double distance = distanceMeters(position, center);
        result.push_back({
            cell,
            grid.linearIndex(cell),
            center,
            distance,
            influenceWeight(distance, radiusM)
        });
    }

    return result;
}

} // namespace

std::vector<MappedPlantPoint> mapPlantsToGrid(
    const std::vector<PlantPoint>& plants,
    const Grid3D& grid
) {
    std::vector<MappedPlantPoint> result;
    result.reserve(plants.size());

    for (const PlantPoint& plant : plants) {
        const GridIndex cell = grid.cellAt(plant.position);
        const Vec3 center = grid.cellCenter(cell);
        result.push_back({
            plant,
            cell,
            grid.linearIndex(cell),
            center,
            distanceMeters(plant.position, center)
        });
    }

    return result;
}

std::vector<MappedVent> mapVentsToGrid(
    const std::vector<VentSpec>& vents,
    const Grid3D& grid
) {
    std::vector<MappedVent> result;
    result.reserve(vents.size());

    for (const VentSpec& vent : vents) {
        const GridIndex anchorCell = grid.cellAt(vent.position);
        result.push_back({
            vent,
            anchorCell,
            grid.linearIndex(anchorCell),
            buildInfluenceCells(grid, vent.position, vent.influenceRadiusM)
        });
    }

    return result;
}

std::vector<MappedHeater> mapHeatersToGrid(
    const std::vector<HeaterSpec>& heaters,
    const Grid3D& grid
) {
    std::vector<MappedHeater> result;
    result.reserve(heaters.size());

    for (const HeaterSpec& heater : heaters) {
        const GridIndex anchorCell = grid.cellAt(heater.position);
        result.push_back({
            heater,
            anchorCell,
            grid.linearIndex(anchorCell),
            buildInfluenceCells(grid, heater.position, heater.influenceRadiusM)
        });
    }

    return result;
}

std::vector<MappedHumidifier> mapHumidifiersToGrid(
    const std::vector<HumidifierSpec>& humidifiers,
    const Grid3D& grid
) {
    std::vector<MappedHumidifier> result;
    result.reserve(humidifiers.size());

    for (const HumidifierSpec& humidifier : humidifiers) {
        const GridIndex anchorCell = grid.cellAt(humidifier.position);
        result.push_back({
            humidifier,
            anchorCell,
            grid.linearIndex(anchorCell),
            buildInfluenceCells(grid, humidifier.position, humidifier.influenceRadiusM)
        });
    }

    return result;
}

} // namespace greenhouse
