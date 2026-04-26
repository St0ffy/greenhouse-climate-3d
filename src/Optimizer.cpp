#include "Optimizer.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace greenhouse {

namespace {

double distance2D(const Vec3& first, const Vec3& second) {
    const double dx = first.x - second.x;
    const double dy = first.y - second.y;
    return std::sqrt(dx * dx + dy * dy);
}

double distanceToNearestPlant(const Vec3& position, const std::vector<PlantPoint>& plants) {
    double best = std::numeric_limits<double>::max();
    for (const PlantPoint& plant : plants) {
        best = std::min(best, distance2D(position, plant.position));
    }
    return best;
}

std::vector<Vec3> generateCandidatePositions(
    const SimulationConfig& config,
    const Grid3D& grid
) {
    const double step = config.optimizer.candidateStepM > 0.0
        ? config.optimizer.candidateStepM
        : 1.0;
    const int requestedMax = config.optimizer.maxCandidates > 0
        ? config.optimizer.maxCandidates
        : 1;
    const std::size_t minRequired =
        std::max<std::size_t>(1, config.heaters.size());
    const std::size_t maxCandidates =
        std::max<std::size_t>(minRequired, static_cast<std::size_t>(requestedMax));
    const double defaultZ = config.heaters.empty()
        ? std::min(0.2, grid.greenhouseSize().height)
        : config.heaters.front().position.z;

    std::vector<Vec3> candidates;
    for (double x = step * 0.5; x < grid.greenhouseSize().length; x += step) {
        for (double y = step * 0.5; y < grid.greenhouseSize().width; y += step) {
            candidates.push_back({
                x,
                y,
                std::clamp(defaultZ, 0.0, grid.greenhouseSize().height)
            });
        }
    }

    if (candidates.empty()) {
        candidates.push_back({
            grid.greenhouseSize().length * 0.5,
            grid.greenhouseSize().width * 0.5,
            std::clamp(defaultZ, 0.0, grid.greenhouseSize().height)
        });
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [&config](const Vec3& left, const Vec3& right) {
            const double leftDistance = distanceToNearestPlant(left, config.plants);
            const double rightDistance = distanceToNearestPlant(right, config.plants);
            if (leftDistance == rightDistance) {
                if (left.x == right.x) {
                    return left.y < right.y;
                }
                return left.x < right.x;
            }
            return leftDistance < rightDistance;
        }
    );

    if (candidates.size() > maxCandidates) {
        candidates.resize(maxCandidates);
    }

    return candidates;
}

std::vector<HeaterSpec> makeHeatersForLayout(
    const std::vector<HeaterSpec>& baseHeaters,
    const std::vector<Vec3>& candidatePositions,
    const std::vector<std::size_t>& chosenIndices
) {
    std::vector<HeaterSpec> heaters = baseHeaters;
    for (std::size_t i = 0; i < heaters.size(); ++i) {
        Vec3 position = candidatePositions[chosenIndices[i]];
        position.z = heaters[i].position.z;
        heaters[i].position = position;
    }
    return heaters;
}

MappedDeviceSet mapDevicesForHeaters(
    const SimulationConfig& config,
    const std::vector<HeaterSpec>& heaters,
    const Grid3D& grid
) {
    return mapDeviceSetToGrid(
        config.plants,
        config.vents,
        heaters,
        config.humidifiers,
        grid
    );
}

std::string buildRecommendation(
    const OptimizationResult& result
) {
    std::ostringstream recommendation;
    recommendation.setf(std::ios::fixed);
    recommendation.precision(2);

    if (!result.improved) {
        recommendation
            << "Baseline heater placement is already as good as or better than tested layouts. "
            << "Keep current heater positions or expand optimizer candidates.";
        return recommendation.str();
    }

    recommendation
        << "Use optimized heater positions. Average plant temperature error improves from "
        << result.baselineScore.averagePlantTemperatureErrorC
        << " C to "
        << result.bestScore.averagePlantTemperatureErrorC
        << " C.";
    return recommendation.str();
}

} // namespace

OptimizationScore scoreSimulation(
    const SimulationResult& simulation,
    double energyWeight
) {
    if (simulation.frames.empty()) {
        return {};
    }

    const SimulationFrame& finalFrame = simulation.frames.back();
    const double averageError = finalFrame.plantTemperature.averageAbsoluteErrorC;
    const double maxError = finalFrame.plantTemperature.maxAbsoluteErrorC;
    const double energy = simulation.totalHeaterEnergyKWh;

    return {
        averageError + 0.25 * maxError + energyWeight * energy,
        averageError,
        maxError,
        energy
    };
}

OptimizationResult optimizeHeaterPlacement(
    const SimulationConfig& config,
    const Grid3D& grid,
    const WeatherTimeline& weather,
    const MaterialProperties& material
) {
    if (config.heaters.empty()) {
        throw std::invalid_argument("optimizer requires at least one heater");
    }

    OptimizationResult result;
    result.candidatePositions = generateCandidatePositions(config, grid);

    result.baselineDevices =
        mapDevicesForHeaters(config, config.heaters, grid);
    result.baselineSimulation =
        runSimulation(config, grid, weather, material, result.baselineDevices);
    result.baselineScore =
        scoreSimulation(result.baselineSimulation, config.optimizer.energyWeight);

    result.bestDevices = result.baselineDevices;
    result.bestSimulation = result.baselineSimulation;
    result.bestScore = result.baselineScore;

    const std::size_t heaterCount = config.heaters.size();
    const std::size_t maxLayouts = config.optimizer.maxLayouts > 0
        ? static_cast<std::size_t>(config.optimizer.maxLayouts)
        : 1;
    std::vector<std::size_t> chosen;

    std::function<void(std::size_t)> search = [&](std::size_t start) {
        if (result.testedLayouts >= maxLayouts) {
            return;
        }

        if (chosen.size() == heaterCount) {
            const std::vector<HeaterSpec> candidateHeaters =
                makeHeatersForLayout(
                    config.heaters,
                    result.candidatePositions,
                    chosen
                );
            const MappedDeviceSet candidateDevices =
                mapDevicesForHeaters(config, candidateHeaters, grid);

            SimulationConfig candidateConfig = config;
            candidateConfig.heaters = candidateHeaters;

            const SimulationResult simulation =
                runSimulation(
                    candidateConfig,
                    grid,
                    weather,
                    material,
                    candidateDevices
                );
            const OptimizationScore score =
                scoreSimulation(simulation, config.optimizer.energyWeight);

            ++result.testedLayouts;
            if (score.quality < result.bestScore.quality) {
                result.bestScore = score;
                result.bestDevices = candidateDevices;
                result.bestSimulation = simulation;
                result.improved = true;
            }
            return;
        }

        const std::size_t remaining = heaterCount - chosen.size();
        if (result.candidatePositions.size() < remaining) {
            return;
        }

        const std::size_t limit =
            result.candidatePositions.size() - remaining + 1;
        for (std::size_t index = start; index < limit; ++index) {
            chosen.push_back(index);
            search(index + 1);
            chosen.pop_back();

            if (result.testedLayouts >= maxLayouts) {
                return;
            }
        }
    };

    search(0);
    result.recommendation = buildRecommendation(result);
    return result;
}

} // namespace greenhouse
