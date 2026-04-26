#include "Devices.h"
#include "Geometry.h"

#include <cassert>
#include <cmath>
#include <vector>

namespace {

bool almostEqual(double left, double right) {
    return std::fabs(left - right) < 1e-9;
}

void assertIndexEquals(const greenhouse::GridIndex& actual, const greenhouse::GridIndex& expected) {
    assert(actual == expected);
}

} // namespace

int main() {
    const greenhouse::Grid3D grid({12.0, 6.0, 3.0}, {12, 6, 4});

    assert(grid.cellCount() == 288);
    assert(almostEqual(grid.cellSizeX(), 1.0));
    assert(almostEqual(grid.cellSizeY(), 1.0));
    assert(almostEqual(grid.cellSizeZ(), 0.75));

    assertIndexEquals(grid.cellAt({0.0, 0.0, 0.0}), {0, 0, 0});
    assertIndexEquals(grid.cellAt({12.0, 6.0, 3.0}), {11, 5, 3});
    assertIndexEquals(grid.cellAt({3.2, 2.1, 0.5}), {3, 2, 0});

    const greenhouse::GridIndex sample{7, 4, 2};
    assert(grid.indexFromLinear(grid.linearIndex(sample)) == sample);

    const greenhouse::Vec3 firstCenter = grid.cellCenter({0, 0, 0});
    assert(almostEqual(firstCenter.x, 0.5));
    assert(almostEqual(firstCenter.y, 0.5));
    assert(almostEqual(firstCenter.z, 0.375));

    assert(grid.neighbors6({5, 3, 2}).size() == 6);
    assert(grid.neighbors6({0, 0, 0}).size() == 3);
    assert(grid.isBoundaryCell({0, 3, 2}));
    assert(!grid.isBoundaryCell({5, 3, 2}));

    const std::vector<greenhouse::GridIndex> radiusCells =
        grid.cellsWithinRadius({3.0, 3.0, 0.2}, 2.5);
    assert(!radiusCells.empty());

    const std::vector<greenhouse::PlantPoint> plants = {
        {"plant", {3.0, 2.0, 0.5}, 22.0}
    };
    const std::vector<greenhouse::MappedPlantPoint> mappedPlants =
        greenhouse::mapPlantsToGrid(plants, grid);
    assert(mappedPlants.size() == 1);
    assertIndexEquals(mappedPlants.front().cell, {3, 2, 0});

    const std::vector<greenhouse::HeaterSpec> heaters = {
        {"heater", {3.0, 3.0, 0.2}, 1200.0, 2.5}
    };
    const std::vector<greenhouse::MappedHeater> mappedHeaters =
        greenhouse::mapHeatersToGrid(heaters, grid);
    assert(mappedHeaters.size() == 1);
    assert(!mappedHeaters.front().influenceCells.empty());

    return 0;
}
