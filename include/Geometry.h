#pragma once

#include "Types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace greenhouse {

double distanceMeters(const Vec3& first, const Vec3& second);

class Grid3D {
public:
    Grid3D(const GreenhouseSize& greenhouseSize, const GridSize& gridSize);

    const GreenhouseSize& greenhouseSize() const;
    const GridSize& gridSize() const;

    double cellSizeX() const;
    double cellSizeY() const;
    double cellSizeZ() const;
    Vec3 cellSize() const;

    std::size_t cellCount() const;
    bool contains(const GridIndex& index) const;
    bool containsPosition(const Vec3& position) const;

    GridIndex cellAt(const Vec3& position) const;
    GridIndex clampedCellAt(const Vec3& position) const;
    Vec3 cellCenter(const GridIndex& index) const;

    std::size_t linearIndex(const GridIndex& index) const;
    GridIndex indexFromLinear(std::size_t linearIndex) const;

    std::vector<GridIndex> neighbors6(const GridIndex& index) const;
    bool isBoundaryCell(const GridIndex& index) const;
    std::vector<GridIndex> cellsWithinRadius(const Vec3& position, double radiusM) const;

    std::string formatIndex(const GridIndex& index) const;

private:
    GreenhouseSize greenhouseSize_;
    GridSize gridSize_;
    Vec3 cellSize_;

    void validateIndex(const GridIndex& index) const;
};

} // namespace greenhouse
