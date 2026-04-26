#include "Physics.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace greenhouse {

namespace {

void validateStateSize(const std::vector<CellState>& cells, const Grid3D& grid) {
    if (cells.size() != grid.cellCount()) {
        throw std::invalid_argument("cell state count does not match grid cell count");
    }
}

void validateTimeStep(double timeStepSeconds) {
    if (timeStepSeconds <= 0.0) {
        throw std::invalid_argument("time step must be positive");
    }
}

double clampTemperature(double temperatureC, const TemperaturePhysicsSettings& settings) {
    return std::clamp(
        temperatureC,
        settings.minTemperatureC,
        settings.maxTemperatureC
    );
}

double neighborAverageTemperature(
    const std::vector<CellState>& cells,
    const Grid3D& grid,
    const GridIndex& index
) {
    const std::vector<GridIndex> neighbors = grid.neighbors6(index);
    if (neighbors.empty()) {
        return cells[grid.linearIndex(index)].temperatureC;
    }

    double total = 0.0;
    for (const GridIndex& neighbor : neighbors) {
        total += cells[grid.linearIndex(neighbor)].temperatureC;
    }

    return total / static_cast<double>(neighbors.size());
}

double heightSolarFactor(const Grid3D& grid, const GridIndex& index) {
    const double normalizedHeight =
        (static_cast<double>(index.z) + 0.5)
        / static_cast<double>(grid.gridSize().nz);

    return 0.30 + 0.70 * normalizedHeight;
}

double sumInfluenceWeights(const std::vector<DeviceInfluenceCell>& cells) {
    double total = 0.0;
    for (const DeviceInfluenceCell& cell : cells) {
        total += cell.weight;
    }
    return total;
}

std::vector<double> buildHeaterDeltas(
    const Grid3D& grid,
    const MappedDeviceSet& devices,
    double timeStepSeconds,
    const TemperaturePhysicsSettings& settings
) {
    std::vector<double> deltas(grid.cellCount(), 0.0);

    for (const MappedHeater& heater : devices.heaters) {
        const double totalWeight = sumInfluenceWeights(heater.influenceCells);
        if (totalWeight <= 0.0 || heater.spec.powerW <= 0.0) {
            continue;
        }

        const double totalGain =
            heater.spec.powerW
            * settings.heaterGainCPerWattSecond
            * timeStepSeconds;

        for (const DeviceInfluenceCell& cell : heater.influenceCells) {
            deltas[cell.linearIndex] += totalGain * (cell.weight / totalWeight);
        }
    }

    return deltas;
}

} // namespace

std::vector<CellState> makeInitialCells(
    const Grid3D& grid,
    double temperatureC,
    double humidityPercent
) {
    const double clampedHumidity = std::clamp(humidityPercent, 0.0, 100.0);
    return std::vector<CellState>(
        grid.cellCount(),
        CellState{temperatureC, clampedHumidity}
    );
}

TemperatureStats summarizeTemperature(const std::vector<CellState>& cells) {
    if (cells.empty()) {
        return {};
    }

    double minTemperature = std::numeric_limits<double>::max();
    double maxTemperature = std::numeric_limits<double>::lowest();
    double totalTemperature = 0.0;

    for (const CellState& cell : cells) {
        minTemperature = std::min(minTemperature, cell.temperatureC);
        maxTemperature = std::max(maxTemperature, cell.temperatureC);
        totalTemperature += cell.temperatureC;
    }

    return {
        minTemperature,
        maxTemperature,
        totalTemperature / static_cast<double>(cells.size())
    };
}

PlantTemperatureStats summarizePlantTemperatures(
    const std::vector<CellState>& cells,
    const std::vector<MappedPlantPoint>& plants
) {
    if (plants.empty()) {
        return {};
    }

    double totalTemperature = 0.0;
    double totalTarget = 0.0;
    double totalAbsError = 0.0;
    double maxAbsError = 0.0;

    for (const MappedPlantPoint& plant : plants) {
        if (plant.linearIndex >= cells.size()) {
            throw std::out_of_range("plant cell index is outside cell state vector");
        }

        const double temperature = cells[plant.linearIndex].temperatureC;
        const double target = plant.plant.targetTemperatureC;
        const double absError = std::fabs(temperature - target);

        totalTemperature += temperature;
        totalTarget += target;
        totalAbsError += absError;
        maxAbsError = std::max(maxAbsError, absError);
    }

    return {
        totalTemperature / static_cast<double>(plants.size()),
        totalTarget / static_cast<double>(plants.size()),
        totalAbsError / static_cast<double>(plants.size()),
        maxAbsError
    };
}

TemperatureStepResult advanceTemperature(
    const std::vector<CellState>& current,
    const Grid3D& grid,
    const WeatherCondition& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices,
    double timeStepSeconds,
    const TemperaturePhysicsSettings& settings
) {
    validateStateSize(current, grid);
    validateTimeStep(timeStepSeconds);

    std::vector<CellState> next = current;
    const std::vector<double> heaterDeltas =
        buildHeaterDeltas(grid, devices, timeStepSeconds, settings);

    double totalNeighborExchangeAbs = 0.0;
    double totalBoundaryLoss = 0.0;
    double totalSolarGain = 0.0;
    double totalHeaterGain = 0.0;

    for (std::size_t linear = 0; linear < current.size(); ++linear) {
        const GridIndex index = grid.indexFromLinear(linear);
        const double temperature = current[linear].temperatureC;

        const double neighborAverage =
            neighborAverageTemperature(current, grid, index);
        const double neighborDelta =
            settings.heatTransferRatePerSecond
            * timeStepSeconds
            * (neighborAverage - temperature);

        double boundaryDelta = 0.0;
        if (grid.isBoundaryCell(index)) {
            boundaryDelta =
                material.heatLossCoefficient
                * settings.boundaryHeatLossRatePerSecond
                * timeStepSeconds
                * (weather.outsideTemperatureC - temperature);
        }

        const double solarDelta =
            weather.solarRadiationWm2
            * material.solarTransmission
            * settings.solarGainCPerWm2Second
            * timeStepSeconds
            * heightSolarFactor(grid, index);

        const double heaterDelta = heaterDeltas[linear];

        next[linear].temperatureC = clampTemperature(
            temperature + neighborDelta + boundaryDelta + solarDelta + heaterDelta,
            settings
        );
        next[linear].humidityPercent = current[linear].humidityPercent;

        totalNeighborExchangeAbs += std::fabs(neighborDelta);
        totalBoundaryLoss += boundaryDelta;
        totalSolarGain += solarDelta;
        totalHeaterGain += heaterDelta;
    }

    const double cellCount = static_cast<double>(current.size());
    TemperatureStepSummary summary;
    summary.temperature = summarizeTemperature(next);
    summary.averageNeighborExchangeAbsC = totalNeighborExchangeAbs / cellCount;
    summary.averageBoundaryLossC = totalBoundaryLoss / cellCount;
    summary.averageSolarGainC = totalSolarGain / cellCount;
    summary.averageHeaterGainC = totalHeaterGain / cellCount;
    summary.heaterEnergyKWh =
        devices.totalHeaterPowerW * timeStepSeconds / 3'600'000.0;

    return {next, summary};
}

} // namespace greenhouse
