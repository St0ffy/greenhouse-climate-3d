#include "Plant.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace greenhouse {

namespace {

double rangeScore(double value, double target, double minValue, double maxValue) {
    if (maxValue <= minValue) {
        return 0.0;
    }

    if (value < minValue || value > maxValue) {
        return 0.0;
    }

    const double span =
        value < target
            ? std::max(1e-9, target - minValue)
            : std::max(1e-9, maxValue - target);
    return std::clamp(1.0 - std::fabs(value - target) / span, 0.0, 1.0);
}

bool outsideSurvivalRange(
    const PlantPoint& plant,
    const PlantSensorReading& sensor
) {
    return sensor.temperatureC < plant.minSurvivalTemperatureC
        || sensor.temperatureC > plant.maxSurvivalTemperatureC
        || sensor.humidityPercent < plant.minSurvivalHumidityPercent
        || sensor.humidityPercent > plant.maxSurvivalHumidityPercent
        || sensor.lightWm2 < plant.minSurvivalLightWm2
        || sensor.lightWm2 > plant.maxSurvivalLightWm2;
}

const PlantSensorReading& sensorForPlant(
    const std::vector<PlantSensorReading>& sensors,
    std::size_t index
) {
    if (index >= sensors.size()) {
        throw std::out_of_range("missing plant sensor reading");
    }
    return sensors[index];
}

} // namespace

std::vector<PlantState> makeInitialPlantStates(
    const std::vector<MappedPlantPoint>& plants
) {
    std::vector<PlantState> result;
    result.reserve(plants.size());

    for (const MappedPlantPoint& plant : plants) {
        result.push_back({
            plant.plant.name,
            std::clamp(plant.plant.initialHealth, 0.0, 1.0),
            std::max(0.0, plant.plant.initialGrowth),
            0.0,
            0.0,
            plant.plant.initialHealth > 0.0
        });
    }

    return result;
}

std::vector<PlantSensorReading> readPlantSensors(
    const std::vector<CellState>& cells,
    const std::vector<MappedPlantPoint>& plants
) {
    std::vector<PlantSensorReading> result;
    result.reserve(plants.size());

    for (const MappedPlantPoint& plant : plants) {
        if (plant.sensorLinearIndex >= cells.size()) {
            throw std::out_of_range("plant sensor cell index is outside cell state vector");
        }

        const CellState& cell = cells[plant.sensorLinearIndex];
        result.push_back({
            plant.plant.name,
            plant.cell,
            plant.sensorCell,
            plant.sensorLinearIndex,
            cell.temperatureC,
            cell.humidityPercent,
            cell.lightWm2
        });
    }

    return result;
}

double plantComfortScore(
    const PlantPoint& plant,
    const PlantSensorReading& sensor
) {
    const double temperatureScore = rangeScore(
        sensor.temperatureC,
        plant.targetTemperatureC,
        plant.minSurvivalTemperatureC,
        plant.maxSurvivalTemperatureC
    );
    const double humidityScore = rangeScore(
        sensor.humidityPercent,
        plant.targetHumidityPercent,
        plant.minSurvivalHumidityPercent,
        plant.maxSurvivalHumidityPercent
    );
    const double lightScore = rangeScore(
        sensor.lightWm2,
        plant.targetLightWm2,
        plant.minSurvivalLightWm2,
        plant.maxSurvivalLightWm2
    );

    return 0.40 * temperatureScore + 0.35 * humidityScore + 0.25 * lightScore;
}

PlantGrowthStats summarizePlantStates(
    const std::vector<PlantState>& plants
) {
    if (plants.empty()) {
        return {};
    }

    double totalHealth = 0.0;
    double minHealth = 1.0;
    double totalGrowth = 0.0;
    double totalComfort = 0.0;
    int aliveCount = 0;

    for (const PlantState& plant : plants) {
        totalHealth += plant.health;
        minHealth = std::min(minHealth, plant.health);
        totalGrowth += plant.growth;
        totalComfort += plant.comfort;
        if (plant.alive) {
            ++aliveCount;
        }
    }

    return {
        totalHealth / static_cast<double>(plants.size()),
        minHealth,
        totalGrowth / static_cast<double>(plants.size()),
        totalComfort / static_cast<double>(plants.size()),
        aliveCount
    };
}

PlantGrowthStepResult advancePlantGrowth(
    const std::vector<PlantState>& current,
    const std::vector<MappedPlantPoint>& plants,
    const std::vector<PlantSensorReading>& sensors,
    double timeStepSeconds
) {
    if (current.size() != plants.size()) {
        throw std::invalid_argument("plant state count does not match mapped plant count");
    }
    if (timeStepSeconds < 0.0) {
        throw std::invalid_argument("plant growth time step cannot be negative");
    }

    std::vector<PlantState> next = current;
    const double hours = timeStepSeconds / 3600.0;

    for (std::size_t i = 0; i < plants.size(); ++i) {
        PlantState& state = next[i];
        const PlantPoint& plant = plants[i].plant;
        const PlantSensorReading& sensor = sensorForPlant(sensors, i);
        const double comfort = plantComfortScore(plant, sensor);
        const bool outsideRange = outsideSurvivalRange(plant, sensor);

        state.name = plant.name;
        state.ageSeconds += timeStepSeconds;
        state.comfort = comfort;

        if (!state.alive) {
            state.health = 0.0;
            continue;
        }

        const double healthDeltaPerHour =
            outsideRange ? -0.30 : (comfort - 0.45) * 0.10;
        state.health = std::clamp(
            state.health + healthDeltaPerHour * hours,
            0.0,
            1.0
        );

        if (state.health <= 0.0) {
            state.alive = false;
            state.health = 0.0;
            continue;
        }

        if (comfort > 0.45) {
            state.growth += comfort * state.health * hours;
        }
    }

    return {next, summarizePlantStates(next)};
}

} // namespace greenhouse
