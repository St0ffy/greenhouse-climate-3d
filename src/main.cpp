#include "Config.h"
#include "Devices.h"
#include "Geometry.h"
#include "Material.h"
#include "Physics.h"
#include "Weather.h"

#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printUsage() {
    std::cout
        << "Usage:\n"
        << "  greenhouse3d simulate <config_path>\n"
        << "  greenhouse3d optimize <config_path>\n";
}

bool isValidMode(const std::string& mode) {
    return mode == "simulate" || mode == "optimize";
}

void printGridSummary(const greenhouse::Grid3D& grid) {
    const greenhouse::Vec3 cellSize = grid.cellSize();

    std::cout << "Cells total: " << grid.cellCount() << "\n";
    std::cout << "Cell size, meters: "
              << cellSize.x << " x " << cellSize.y << " x " << cellSize.z << "\n";
}

void printPlantMapping(
    const std::vector<greenhouse::MappedPlantPoint>& plants,
    const greenhouse::Grid3D& grid
) {
    std::cout << "Plant control points: " << plants.size() << "\n";
    for (const greenhouse::MappedPlantPoint& plant : plants) {
        std::cout << "  " << plant.plant.name
                  << " -> cell " << grid.formatIndex(plant.cell)
                  << ", linear " << plant.linearIndex
                  << ", target " << plant.plant.targetTemperatureC << " C\n";
    }
}

void printDeviceMapping(
    const greenhouse::MappedDeviceSet& devices,
    const greenhouse::Grid3D& grid
) {
    std::cout << "Vents: " << devices.vents.size()
              << ", average opening " << devices.averageVentOpening << "\n";
    for (const greenhouse::MappedVent& vent : devices.vents) {
        std::cout << "  " << vent.spec.name
                  << " -> cell " << grid.formatIndex(vent.anchorCell)
                  << ", opening " << vent.spec.opening
                  << ", affected cells " << vent.influenceCells.size() << "\n";
    }

    std::cout << "Heaters: " << devices.heaters.size()
              << ", total power " << devices.totalHeaterPowerW << " W\n";
    for (const greenhouse::MappedHeater& heater : devices.heaters) {
        std::cout << "  " << heater.spec.name
                  << " -> cell " << grid.formatIndex(heater.anchorCell)
                  << ", power " << heater.spec.powerW
                  << " W, affected cells " << heater.influenceCells.size() << "\n";
    }

    std::cout << "Humidifiers: " << devices.humidifiers.size() << "\n";
    for (const greenhouse::MappedHumidifier& humidifier : devices.humidifiers) {
        std::cout << "  " << humidifier.spec.name
                  << " -> cell " << grid.formatIndex(humidifier.anchorCell)
                  << ", mode " << humidifier.spec.mode
                  << ", multiplier " << greenhouse::humidifierModeMultiplier(humidifier.spec.mode)
                  << ", affected cells " << humidifier.influenceCells.size() << "\n";
    }
}

void printWeatherSummary(
    const greenhouse::WeatherTimeline& weather,
    double durationSeconds
) {
    const greenhouse::WeatherCondition start = weather.at(0.0);
    const greenhouse::WeatherCondition middle = weather.at(durationSeconds / 2.0);
    const greenhouse::WeatherCondition end = weather.at(durationSeconds);

    std::cout << "Weather source: " << weather.source()
              << ", samples " << weather.sampleCount() << "\n";
    std::cout << "  t=0s: temp " << start.outsideTemperatureC
              << " C, humidity " << start.outsideHumidityPercent
              << " %, solar " << start.solarRadiationWm2 << " W/m2\n";
    std::cout << "  t=mid: temp " << middle.outsideTemperatureC
              << " C, humidity " << middle.outsideHumidityPercent
              << " %, solar " << middle.solarRadiationWm2 << " W/m2\n";
    std::cout << "  t=end: temp " << end.outsideTemperatureC
              << " C, humidity " << end.outsideHumidityPercent
              << " %, solar " << end.solarRadiationWm2 << " W/m2\n";
}

void printMaterialSummary(const greenhouse::MaterialProperties& material) {
    std::cout << "Material: " << material.name
              << ", heat loss coefficient " << material.heatLossCoefficient
              << ", solar transmission " << material.solarTransmission << "\n";
}

void printClimatePreview(
    const greenhouse::Grid3D& grid,
    const greenhouse::SimulationConfig& config,
    const greenhouse::WeatherTimeline& weather,
    const greenhouse::MaterialProperties& material,
    const greenhouse::MappedDeviceSet& devices
) {
    const std::vector<greenhouse::CellState> initialCells =
        greenhouse::makeInitialCells(
            grid,
            config.initialTemperatureC,
            config.initialHumidityPercent
        );
    greenhouse::ClimatePhysicsSettings settings;
    settings.humidity.humidityEnabled = config.humidityEnabled;

    const greenhouse::ClimateStepResult preview =
        greenhouse::advanceClimate(
            initialCells,
            grid,
            weather.at(0.0),
            material,
            devices,
            config.timeStepSeconds,
            settings
        );
    const greenhouse::PlantTemperatureStats plantStats =
        greenhouse::summarizePlantTemperatures(preview.cells, devices.plants);
    const greenhouse::PlantHumidityStats plantHumidity =
        greenhouse::summarizePlantHumidity(preview.cells, devices.plants);

    std::cout << "One-step climate preview:\n";
    std::cout << "  average " << preview.summary.temperatureStep.temperature.averageTemperatureC
              << " C, min " << preview.summary.temperatureStep.temperature.minTemperatureC
              << " C, max " << preview.summary.temperatureStep.temperature.maxTemperatureC << " C\n";
    std::cout << "  avg solar gain " << preview.summary.temperatureStep.averageSolarGainC
              << " C, avg heater gain " << preview.summary.temperatureStep.averageHeaterGainC
              << " C, avg boundary delta " << preview.summary.temperatureStep.averageBoundaryLossC
              << " C, avg vent delta " << preview.summary.averageVentTemperatureDeltaC << " C\n";
    std::cout << "  humidity average " << preview.summary.humidity.averageHumidityPercent
              << " %, min " << preview.summary.humidity.minHumidityPercent
              << " %, max " << preview.summary.humidity.maxHumidityPercent << " %\n";
    std::cout << "  avg humidifier gain " << preview.summary.averageHumidifierGainPercent
              << " %, avg humidity diffusion " << preview.summary.averageHumidityExchangeAbsPercent
              << " %, avg vent humidity delta " << preview.summary.averageVentHumidityDeltaPercent << " %\n";
    std::cout << "  plant average " << plantStats.averageTemperatureC
              << " C, target " << plantStats.averageTargetTemperatureC
              << " C, average error " << plantStats.averageAbsoluteErrorC
              << " C, plant humidity " << plantHumidity.averageHumidityPercent << " %\n";
    std::cout << "  heater energy for one step "
              << preview.summary.temperatureStep.heaterEnergyKWh << " kWh\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printUsage();
        return 1;
    }

    const std::string mode = argv[1];
    const std::string configPath = argv[2];

    if (!isValidMode(mode)) {
        std::cerr << "Unknown mode: " << mode << "\n";
        printUsage();
        return 1;
    }

    try {
        greenhouse::SimulationConfig config = greenhouse::loadConfig(configPath);
        config.mode = mode;
        greenhouse::Grid3D grid(config.greenhouseSize, config.gridSize);
        const greenhouse::MappedDeviceSet devices =
            greenhouse::mapDeviceSetToGrid(
                config.plants,
                config.vents,
                config.heaters,
                config.humidifiers,
                grid
            );
        const greenhouse::MaterialProperties material =
            greenhouse::makeMaterial(config.material);
        const greenhouse::WeatherTimeline weather =
            greenhouse::WeatherTimeline::fromCsv(
                config.weather.weatherFile,
                config.weather
            );

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Greenhouse Climate 3D\n";
        std::cout << "Mode: " << config.mode << "\n";
        std::cout << "Config: " << configPath << "\n";
        std::cout << "Grid: "
                  << config.gridSize.nx << " x "
                  << config.gridSize.ny << " x "
                  << config.gridSize.nz << "\n";
        std::cout << "Duration, seconds: " << config.durationSeconds << "\n";
        std::cout << "Time step, seconds: " << config.timeStepSeconds << "\n";
        printGridSummary(grid);
        printMaterialSummary(material);
        printWeatherSummary(weather, config.durationSeconds);
        printPlantMapping(devices.plants, grid);
        printDeviceMapping(devices, grid);
        printClimatePreview(grid, config, weather, material, devices);
        std::cout << "\nDay 5 humidity and ventilation physics is ready. Full simulation loop will be added next.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Startup error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
