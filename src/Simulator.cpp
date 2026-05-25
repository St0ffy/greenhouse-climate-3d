#include "Simulator.h"

#include <algorithm>
#include <stdexcept>

namespace greenhouse {

namespace {

void validateSimulationConfig(const SimulationConfig& config) {
    if (config.durationSeconds < 0.0) {
        throw std::invalid_argument("simulation duration cannot be negative");
    }

    if (config.timeStepSeconds <= 0.0) {
        throw std::invalid_argument("simulation time step must be positive");
    }
}

ClimateStepSummary makeInitialSummary(const std::vector<CellState>& cells) {
    ClimateStepSummary summary;
    summary.temperatureStep.temperature = summarizeTemperature(cells);
    summary.humidity = summarizeHumidity(cells);
    return summary;
}

bool samePlanCell(const GridIndex& left, const GridIndex& right) {
    return left.x == right.x && left.y == right.y;
}

int failedDeviceCount(const MappedDeviceSet& devices) {
    int count = 0;
    for (const MappedHeater& heater : devices.heaters) {
        if (heater.spec.failed) {
            ++count;
        }
    }
    for (const MappedHumidifier& humidifier : devices.humidifiers) {
        if (humidifier.spec.failed) {
            ++count;
        }
    }
    for (const MappedVent& vent : devices.vents) {
        if (vent.spec.failed) {
            ++count;
        }
    }
    return count;
}

int activeHumidifierCount(const MappedDeviceSet& devices) {
    int count = 0;
    for (const MappedHumidifier& humidifier : devices.humidifiers) {
        if (humidifier.spec.enabled
            && !humidifier.spec.failed
            && humidifierModeMultiplier(humidifier.spec.mode) > 0.0
            && humidifier.spec.level > 0.0) {
            ++count;
        }
    }
    return count;
}

} // namespace

SimulationStepper::SimulationStepper(
    const SimulationConfig& config,
    const Grid3D& grid,
    const WeatherTimeline& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices,
    const ClimatePhysicsSettings& settings
) : config_(config),
    grid_(grid),
    weather_(weather),
    material_(material),
    settings_(settings),
    controller_(config.control),
    devices_(devices) {
    validateSimulationConfig(config_);

    settings_.humidity.humidityEnabled = config_.humidityEnabled;
    refreshDeviceRuntimeTotals(devices_);

    cells_ =
        makeInitialCells(
            grid_,
            config_.initialTemperatureC,
            config_.initialHumidityPercent
        );
    updateInitialLight();
    plantStates_ = makeInitialPlantStates(devices_.plants);

    result_.config = config_;
    result_.material = material_;
    result_.durationSeconds = config_.durationSeconds;
    result_.timeStepSeconds = config_.timeStepSeconds;

    ControlStepSummary initialControl;
    initialControl.enabled = config_.control.enabled;
    initialControl.mlEnabled = config_.control.mlEnabled;
    initialControl.activeHeaterPowerW = devices_.totalHeaterPowerW;
    initialControl.averageVentOpening = devices_.averageVentOpening;
    initialControl.activeHumidifierCount = activeHumidifierCount(devices_);
    initialControl.failedDeviceCount = failedDeviceCount(devices_);

    appendFrame(weather_.at(currentTime_), makeInitialSummary(cells_), initialControl);
}

void SimulationStepper::updateInitialLight() {
    const WeatherCondition weather = weather_.at(currentTime_);
    for (std::size_t linear = 0; linear < cells_.size(); ++linear) {
        const GridIndex index = grid_.indexFromLinear(linear);
        cells_[linear].lightWm2 =
            estimateCellLightWm2(grid_, index, weather, material_);
    }
}

void SimulationStepper::appendFrame(
    const WeatherCondition& weather,
    const ClimateStepSummary& climate,
    const ControlStepSummary& control
) {
    const std::vector<PlantSensorReading> sensors =
        readPlantSensors(cells_, devices_.plants);

    result_.frames.push_back({
        currentTime_,
        weather,
        climate,
        summarizePlantTemperatures(cells_, devices_.plants),
        summarizePlantHumidity(cells_, devices_.plants),
        summarizePlantStates(plantStates_),
        control,
        cumulativeHeaterEnergy_,
        cumulativeDeviceEnergy_,
        cells_,
        plantStates_,
        sensors,
        devices_
    });
}

bool SimulationStepper::finished() const {
    return currentTime_ + 1e-9 >= config_.durationSeconds;
}

void SimulationStepper::step() {
    if (finished()) {
        return;
    }

    const double stepSeconds =
        std::min(config_.timeStepSeconds, config_.durationSeconds - currentTime_);
    if (stepSeconds <= 0.0) {
        return;
    }

    const std::vector<PlantSensorReading> sensors =
        readPlantSensors(cells_, devices_.plants);
    ControlStepSummary control =
        controller_.apply(devices_, devices_.plants, sensors, plantStates_);

    const WeatherCondition stepWeather = weather_.at(currentTime_);
    const ClimateStepResult climate =
        advanceClimate(
            cells_,
            grid_,
            stepWeather,
            material_,
            devices_,
            stepSeconds,
            settings_
        );

    cells_ = climate.cells;
    currentTime_ += stepSeconds;
    cumulativeHeaterEnergy_ += climate.summary.temperatureStep.heaterEnergyKWh;
    cumulativeDeviceEnergy_ += climate.summary.totalDeviceEnergyKWh;

    const std::vector<PlantSensorReading> nextSensors =
        readPlantSensors(cells_, devices_.plants);
    const PlantGrowthStepResult plantGrowth =
        advancePlantGrowth(
            plantStates_,
            devices_.plants,
            nextSensors,
            stepSeconds
        );
    plantStates_ = plantGrowth.plants;

    controller_.learn(plantGrowth.stats, climate.summary.totalDeviceEnergyKWh);
    control.reward = controller_.lastReward();
    control.activeHeaterPowerW = devices_.totalHeaterPowerW;
    control.averageVentOpening = devices_.averageVentOpening;
    control.activeHumidifierCount = activeHumidifierCount(devices_);
    control.failedDeviceCount = failedDeviceCount(devices_);

    result_.totalHeaterEnergyKWh = cumulativeHeaterEnergy_;
    result_.totalDeviceEnergyKWh = cumulativeDeviceEnergy_;

    appendFrame(weather_.at(currentTime_), climate.summary, control);
}

bool SimulationStepper::toggleDeviceFailureAt(const GridIndex& cell) {
    for (MappedHeater& heater : devices_.heaters) {
        if (samePlanCell(heater.anchorCell, cell)) {
            heater.spec.failed = !heater.spec.failed;
            refreshDeviceRuntimeTotals(devices_);
            result_.frames.back().devices = devices_;
            result_.frames.back().control.failedDeviceCount = failedDeviceCount(devices_);
            result_.frames.back().control.activeHeaterPowerW = devices_.totalHeaterPowerW;
            return true;
        }
    }

    for (MappedHumidifier& humidifier : devices_.humidifiers) {
        if (samePlanCell(humidifier.anchorCell, cell)) {
            humidifier.spec.failed = !humidifier.spec.failed;
            refreshDeviceRuntimeTotals(devices_);
            result_.frames.back().devices = devices_;
            result_.frames.back().control.failedDeviceCount = failedDeviceCount(devices_);
            result_.frames.back().control.activeHumidifierCount =
                activeHumidifierCount(devices_);
            return true;
        }
    }

    for (MappedVent& vent : devices_.vents) {
        if (samePlanCell(vent.anchorCell, cell)) {
            vent.spec.failed = !vent.spec.failed;
            refreshDeviceRuntimeTotals(devices_);
            result_.frames.back().devices = devices_;
            result_.frames.back().control.failedDeviceCount = failedDeviceCount(devices_);
            result_.frames.back().control.averageVentOpening = devices_.averageVentOpening;
            return true;
        }
    }

    return false;
}

const SimulationResult& SimulationStepper::result() const {
    return result_;
}

const SimulationFrame& SimulationStepper::currentFrame() const {
    return result_.frames.back();
}

const MappedDeviceSet& SimulationStepper::devices() const {
    return devices_;
}

double SimulationStepper::currentTimeSeconds() const {
    return currentTime_;
}

SimulationResult runSimulation(
    const SimulationConfig& config,
    const Grid3D& grid,
    const WeatherTimeline& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices,
    const ClimatePhysicsSettings& settings
) {
    SimulationStepper stepper(config, grid, weather, material, devices, settings);
    while (!stepper.finished()) {
        stepper.step();
    }
    return stepper.result();
}

} // namespace greenhouse
