#include "Devices.h"

#include <algorithm>
#include <cctype>
#include <numeric>
#include <stdexcept>
#include <string>

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

std::string normalizedMode(const std::string& mode) {
    std::string result = mode;
    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        }
    );
    return result;
}

double totalPower(const std::vector<MappedHeater>& heaters) {
    double total = 0.0;
    for (const MappedHeater& heater : heaters) {
        total += heater.spec.powerW;
    }
    return total;
}

double averageOpening(const std::vector<MappedVent>& vents) {
    if (vents.empty()) {
        return 0.0;
    }

    double total = 0.0;
    for (const MappedVent& vent : vents) {
        total += vent.spec.opening;
    }
    return total / static_cast<double>(vents.size());
}

} // namespace

bool isValidHumidifierMode(const std::string& mode) {
    const std::string normalized = normalizedMode(mode);
    return normalized == "off"
        || normalized == "low"
        || normalized == "medium"
        || normalized == "high";
}

double humidifierModeMultiplier(const std::string& mode) {
    const std::string normalized = normalizedMode(mode);
    if (normalized == "off") {
        return 0.0;
    }
    if (normalized == "low") {
        return 0.5;
    }
    if (normalized == "medium") {
        return 1.0;
    }
    if (normalized == "high") {
        return 1.5;
    }

    throw std::invalid_argument("unknown humidifier mode: " + mode);
}

void validateDeviceSpecs(
    const std::vector<PlantPoint>& plants,
    const std::vector<VentSpec>& vents,
    const std::vector<HeaterSpec>& heaters,
    const std::vector<HumidifierSpec>& humidifiers
) {
    if (plants.empty()) {
        throw std::invalid_argument("at least one plant control point is required");
    }

    for (const PlantPoint& plant : plants) {
        if (plant.targetTemperatureC < -50.0 || plant.targetTemperatureC > 80.0) {
            throw std::invalid_argument("plant target temperature is outside a reasonable range");
        }
    }

    for (const VentSpec& vent : vents) {
        if (vent.opening < 0.0 || vent.opening > 1.0) {
            throw std::invalid_argument("vent opening must be in range 0..1: " + vent.name);
        }
        if (vent.influenceRadiusM < 0.0) {
            throw std::invalid_argument("vent influence radius cannot be negative: " + vent.name);
        }
    }

    for (const HeaterSpec& heater : heaters) {
        if (heater.powerW < 0.0) {
            throw std::invalid_argument("heater power cannot be negative: " + heater.name);
        }
        if (heater.influenceRadiusM < 0.0) {
            throw std::invalid_argument("heater influence radius cannot be negative: " + heater.name);
        }
    }

    for (const HumidifierSpec& humidifier : humidifiers) {
        if (!isValidHumidifierMode(humidifier.mode)) {
            throw std::invalid_argument("humidifier mode must be off, low, medium, or high: " + humidifier.name);
        }
        if (humidifier.influenceRadiusM < 0.0) {
            throw std::invalid_argument("humidifier influence radius cannot be negative: " + humidifier.name);
        }
    }
}

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

MappedDeviceSet mapDeviceSetToGrid(
    const std::vector<PlantPoint>& plants,
    const std::vector<VentSpec>& vents,
    const std::vector<HeaterSpec>& heaters,
    const std::vector<HumidifierSpec>& humidifiers,
    const Grid3D& grid
) {
    validateDeviceSpecs(plants, vents, heaters, humidifiers);

    MappedDeviceSet result;
    result.plants = mapPlantsToGrid(plants, grid);
    result.vents = mapVentsToGrid(vents, grid);
    result.heaters = mapHeatersToGrid(heaters, grid);
    result.humidifiers = mapHumidifiersToGrid(humidifiers, grid);
    result.totalHeaterPowerW = totalPower(result.heaters);
    result.averageVentOpening = averageOpening(result.vents);
    return result;
}

} // namespace greenhouse
