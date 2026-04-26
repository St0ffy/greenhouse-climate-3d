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

double clampHumidity(double humidityPercent) {
    return std::clamp(humidityPercent, 0.0, 100.0);
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

double neighborAverageHumidity(
    const std::vector<CellState>& cells,
    const Grid3D& grid,
    const GridIndex& index
) {
    const std::vector<GridIndex> neighbors = grid.neighbors6(index);
    if (neighbors.empty()) {
        return cells[grid.linearIndex(index)].humidityPercent;
    }

    double total = 0.0;
    for (const GridIndex& neighbor : neighbors) {
        total += cells[grid.linearIndex(neighbor)].humidityPercent;
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

std::vector<double> buildHumidifierDeltas(
    const Grid3D& grid,
    const MappedDeviceSet& devices,
    double timeStepSeconds,
    const HumidityPhysicsSettings& settings
) {
    std::vector<double> deltas(grid.cellCount(), 0.0);

    if (!settings.humidityEnabled) {
        return deltas;
    }

    for (const MappedHumidifier& humidifier : devices.humidifiers) {
        const double modeMultiplier = humidifierModeMultiplier(humidifier.spec.mode);
        const double totalWeight = sumInfluenceWeights(humidifier.influenceCells);
        if (totalWeight <= 0.0 || modeMultiplier <= 0.0) {
            continue;
        }

        const double totalGain =
            modeMultiplier
            * settings.humidifierGainPercentPerSecond
            * timeStepSeconds;

        for (const DeviceInfluenceCell& cell : humidifier.influenceCells) {
            deltas[cell.linearIndex] += totalGain * (cell.weight / totalWeight);
        }
    }

    return deltas;
}

double ventMixFactor(
    const MappedVent& vent,
    const DeviceInfluenceCell& cell,
    double ratePerSecond,
    double timeStepSeconds,
    double maxMix
) {
    return std::clamp(
        vent.spec.opening * cell.weight * ratePerSecond * timeStepSeconds,
        0.0,
        maxMix
    );
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

HumidityStats summarizeHumidity(const std::vector<CellState>& cells) {
    if (cells.empty()) {
        return {};
    }

    double minHumidity = std::numeric_limits<double>::max();
    double maxHumidity = std::numeric_limits<double>::lowest();
    double totalHumidity = 0.0;

    for (const CellState& cell : cells) {
        minHumidity = std::min(minHumidity, cell.humidityPercent);
        maxHumidity = std::max(maxHumidity, cell.humidityPercent);
        totalHumidity += cell.humidityPercent;
    }

    return {
        minHumidity,
        maxHumidity,
        totalHumidity / static_cast<double>(cells.size())
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

PlantHumidityStats summarizePlantHumidity(
    const std::vector<CellState>& cells,
    const std::vector<MappedPlantPoint>& plants
) {
    if (plants.empty()) {
        return {};
    }

    double minHumidity = std::numeric_limits<double>::max();
    double maxHumidity = std::numeric_limits<double>::lowest();
    double totalHumidity = 0.0;

    for (const MappedPlantPoint& plant : plants) {
        if (plant.linearIndex >= cells.size()) {
            throw std::out_of_range("plant cell index is outside cell state vector");
        }

        const double humidity = cells[plant.linearIndex].humidityPercent;
        minHumidity = std::min(minHumidity, humidity);
        maxHumidity = std::max(maxHumidity, humidity);
        totalHumidity += humidity;
    }

    return {
        totalHumidity / static_cast<double>(plants.size()),
        minHumidity,
        maxHumidity
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

ClimateStepResult advanceClimate(
    const std::vector<CellState>& current,
    const Grid3D& grid,
    const WeatherCondition& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices,
    double timeStepSeconds,
    const ClimatePhysicsSettings& settings
) {
    validateStateSize(current, grid);
    validateTimeStep(timeStepSeconds);

    const TemperatureStepResult temperatureStep =
        advanceTemperature(
            current,
            grid,
            weather,
            material,
            devices,
            timeStepSeconds,
            settings.temperature
        );

    std::vector<CellState> next = temperatureStep.cells;
    double totalHumidityExchangeAbs = 0.0;
    double totalHumidifierGain = 0.0;
    double totalVentTemperatureDelta = 0.0;
    double totalVentHumidityDelta = 0.0;

    if (settings.humidity.humidityEnabled) {
        const std::vector<double> humidifierDeltas =
            buildHumidifierDeltas(grid, devices, timeStepSeconds, settings.humidity);

        for (std::size_t linear = 0; linear < current.size(); ++linear) {
            const GridIndex index = grid.indexFromLinear(linear);
            const double humidity = current[linear].humidityPercent;
            const double neighborAverage = neighborAverageHumidity(current, grid, index);
            const double diffusionDelta =
                settings.humidity.humidityTransferRatePerSecond
                * timeStepSeconds
                * (neighborAverage - humidity);
            const double humidifierDelta = humidifierDeltas[linear];

            next[linear].humidityPercent =
                clampHumidity(humidity + diffusionDelta + humidifierDelta);

            totalHumidityExchangeAbs += std::fabs(diffusionDelta);
            totalHumidifierGain += humidifierDelta;
        }
    } else {
        for (std::size_t linear = 0; linear < current.size(); ++linear) {
            next[linear].humidityPercent = current[linear].humidityPercent;
        }
    }

    for (const MappedVent& vent : devices.vents) {
        if (vent.spec.opening <= 0.0) {
            continue;
        }

        for (const DeviceInfluenceCell& cell : vent.influenceCells) {
            const double temperatureMix = ventMixFactor(
                vent,
                cell,
                settings.humidity.ventTemperatureExchangeRatePerSecond,
                timeStepSeconds,
                settings.humidity.maxVentilationMixPerStep
            );
            const double temperatureDelta =
                temperatureMix
                * (weather.outsideTemperatureC - next[cell.linearIndex].temperatureC);
            next[cell.linearIndex].temperatureC = clampTemperature(
                next[cell.linearIndex].temperatureC + temperatureDelta,
                settings.temperature
            );
            totalVentTemperatureDelta += temperatureDelta;

            if (settings.humidity.humidityEnabled) {
                const double humidityMix = ventMixFactor(
                    vent,
                    cell,
                    settings.humidity.ventHumidityExchangeRatePerSecond,
                    timeStepSeconds,
                    settings.humidity.maxVentilationMixPerStep
                );
                const double humidityDelta =
                    humidityMix
                    * (weather.outsideHumidityPercent - next[cell.linearIndex].humidityPercent);
                next[cell.linearIndex].humidityPercent =
                    clampHumidity(next[cell.linearIndex].humidityPercent + humidityDelta);
                totalVentHumidityDelta += humidityDelta;
            }
        }
    }

    const double cellCount = static_cast<double>(current.size());
    ClimateStepSummary summary;
    summary.temperatureStep = temperatureStep.summary;
    summary.temperatureStep.temperature = summarizeTemperature(next);
    summary.humidity = summarizeHumidity(next);
    summary.averageHumidityExchangeAbsPercent = totalHumidityExchangeAbs / cellCount;
    summary.averageHumidifierGainPercent = totalHumidifierGain / cellCount;
    summary.averageVentTemperatureDeltaC = totalVentTemperatureDelta / cellCount;
    summary.averageVentHumidityDeltaPercent = totalVentHumidityDelta / cellCount;

    return {next, summary};
}

} // namespace greenhouse
