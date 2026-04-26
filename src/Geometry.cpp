#include "Geometry.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace greenhouse {

namespace {

void validateGreenhouseSize(const GreenhouseSize& size) {
    if (size.length <= 0.0 || size.width <= 0.0 || size.height <= 0.0) {
        throw std::invalid_argument("greenhouse dimensions must be positive");
    }
}

void validateGridSize(const GridSize& size) {
    if (size.nx <= 0 || size.ny <= 0 || size.nz <= 0) {
        throw std::invalid_argument("grid dimensions must be positive");
    }
}

int coordinateToAxisIndex(double coordinate, double fullSize, int cellCount, double cellSize) {
    if (coordinate >= fullSize) {
        return cellCount - 1;
    }

    const int rawIndex = static_cast<int>(std::floor(coordinate / cellSize));
    return std::clamp(rawIndex, 0, cellCount - 1);
}

Vec3 calculateCellSize(const GreenhouseSize& greenhouseSize, const GridSize& gridSize) {
    validateGreenhouseSize(greenhouseSize);
    validateGridSize(gridSize);

    return {
        greenhouseSize.length / gridSize.nx,
        greenhouseSize.width / gridSize.ny,
        greenhouseSize.height / gridSize.nz
    };
}

} // namespace

double distanceMeters(const Vec3& first, const Vec3& second) {
    const double dx = first.x - second.x;
    const double dy = first.y - second.y;
    const double dz = first.z - second.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Grid3D::Grid3D(const GreenhouseSize& greenhouseSize, const GridSize& gridSize)
    : greenhouseSize_(greenhouseSize),
      gridSize_(gridSize),
      cellSize_(calculateCellSize(greenhouseSize, gridSize)) {
}

const GreenhouseSize& Grid3D::greenhouseSize() const {
    return greenhouseSize_;
}

const GridSize& Grid3D::gridSize() const {
    return gridSize_;
}

double Grid3D::cellSizeX() const {
    return cellSize_.x;
}

double Grid3D::cellSizeY() const {
    return cellSize_.y;
}

double Grid3D::cellSizeZ() const {
    return cellSize_.z;
}

Vec3 Grid3D::cellSize() const {
    return cellSize_;
}

std::size_t Grid3D::cellCount() const {
    return static_cast<std::size_t>(gridSize_.nx)
        * static_cast<std::size_t>(gridSize_.ny)
        * static_cast<std::size_t>(gridSize_.nz);
}

bool Grid3D::contains(const GridIndex& index) const {
    return index.x >= 0 && index.x < gridSize_.nx
        && index.y >= 0 && index.y < gridSize_.ny
        && index.z >= 0 && index.z < gridSize_.nz;
}

bool Grid3D::containsPosition(const Vec3& position) const {
    return position.x >= 0.0 && position.x <= greenhouseSize_.length
        && position.y >= 0.0 && position.y <= greenhouseSize_.width
        && position.z >= 0.0 && position.z <= greenhouseSize_.height;
}

GridIndex Grid3D::cellAt(const Vec3& position) const {
    if (!containsPosition(position)) {
        throw std::out_of_range("position is outside greenhouse volume");
    }

    return {
        coordinateToAxisIndex(position.x, greenhouseSize_.length, gridSize_.nx, cellSize_.x),
        coordinateToAxisIndex(position.y, greenhouseSize_.width, gridSize_.ny, cellSize_.y),
        coordinateToAxisIndex(position.z, greenhouseSize_.height, gridSize_.nz, cellSize_.z)
    };
}

GridIndex Grid3D::clampedCellAt(const Vec3& position) const {
    const Vec3 clampedPosition{
        std::clamp(position.x, 0.0, greenhouseSize_.length),
        std::clamp(position.y, 0.0, greenhouseSize_.width),
        std::clamp(position.z, 0.0, greenhouseSize_.height)
    };

    return cellAt(clampedPosition);
}

Vec3 Grid3D::cellCenter(const GridIndex& index) const {
    validateIndex(index);

    return {
        (static_cast<double>(index.x) + 0.5) * cellSize_.x,
        (static_cast<double>(index.y) + 0.5) * cellSize_.y,
        (static_cast<double>(index.z) + 0.5) * cellSize_.z
    };
}

std::size_t Grid3D::linearIndex(const GridIndex& index) const {
    validateIndex(index);

    return static_cast<std::size_t>(index.x)
        + static_cast<std::size_t>(gridSize_.nx)
        * (static_cast<std::size_t>(index.y)
        + static_cast<std::size_t>(gridSize_.ny) * static_cast<std::size_t>(index.z));
}

GridIndex Grid3D::indexFromLinear(std::size_t linearIndexValue) const {
    if (linearIndexValue >= cellCount()) {
        throw std::out_of_range("linear cell index is outside grid");
    }

    const int nx = gridSize_.nx;
    const int ny = gridSize_.ny;
    const int x = static_cast<int>(linearIndexValue % nx);
    const int yz = static_cast<int>(linearIndexValue / nx);
    const int y = yz % ny;
    const int z = yz / ny;

    return {x, y, z};
}

std::vector<GridIndex> Grid3D::neighbors6(const GridIndex& index) const {
    validateIndex(index);

    const std::vector<GridIndex> candidates = {
        {index.x - 1, index.y, index.z},
        {index.x + 1, index.y, index.z},
        {index.x, index.y - 1, index.z},
        {index.x, index.y + 1, index.z},
        {index.x, index.y, index.z - 1},
        {index.x, index.y, index.z + 1}
    };

    std::vector<GridIndex> result;
    result.reserve(candidates.size());
    for (const GridIndex& candidate : candidates) {
        if (contains(candidate)) {
            result.push_back(candidate);
        }
    }

    return result;
}

bool Grid3D::isBoundaryCell(const GridIndex& index) const {
    validateIndex(index);

    return index.x == 0 || index.x == gridSize_.nx - 1
        || index.y == 0 || index.y == gridSize_.ny - 1
        || index.z == 0 || index.z == gridSize_.nz - 1;
}

std::vector<GridIndex> Grid3D::cellsWithinRadius(const Vec3& position, double radiusM) const {
    if (radiusM < 0.0) {
        throw std::invalid_argument("influence radius cannot be negative");
    }

    const GridIndex anchor = cellAt(position);
    if (radiusM == 0.0) {
        return {anchor};
    }

    std::vector<GridIndex> result;
    for (std::size_t linear = 0; linear < cellCount(); ++linear) {
        const GridIndex index = indexFromLinear(linear);
        const Vec3 center = cellCenter(index);
        if (distanceMeters(position, center) <= radiusM) {
            result.push_back(index);
        }
    }

    if (result.empty()) {
        result.push_back(anchor);
    }

    return result;
}

std::string Grid3D::formatIndex(const GridIndex& index) const {
    validateIndex(index);

    std::ostringstream output;
    output << "(" << index.x << ", " << index.y << ", " << index.z << ")";
    return output.str();
}

void Grid3D::validateIndex(const GridIndex& index) const {
    if (!contains(index)) {
        throw std::out_of_range("grid index is outside grid");
    }
}

} // namespace greenhouse
