#include "Config.h"
#include "Devices.h"
#include "Geometry.h"

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
    const std::vector<greenhouse::MappedVent>& vents,
    const std::vector<greenhouse::MappedHeater>& heaters,
    const std::vector<greenhouse::MappedHumidifier>& humidifiers,
    const greenhouse::Grid3D& grid
) {
    std::cout << "Vents: " << vents.size() << "\n";
    for (const greenhouse::MappedVent& vent : vents) {
        std::cout << "  " << vent.spec.name
                  << " -> cell " << grid.formatIndex(vent.anchorCell)
                  << ", opening " << vent.spec.opening
                  << ", affected cells " << vent.influenceCells.size() << "\n";
    }

    std::cout << "Heaters: " << heaters.size() << "\n";
    for (const greenhouse::MappedHeater& heater : heaters) {
        std::cout << "  " << heater.spec.name
                  << " -> cell " << grid.formatIndex(heater.anchorCell)
                  << ", power " << heater.spec.powerW
                  << " W, affected cells " << heater.influenceCells.size() << "\n";
    }

    std::cout << "Humidifiers: " << humidifiers.size() << "\n";
    for (const greenhouse::MappedHumidifier& humidifier : humidifiers) {
        std::cout << "  " << humidifier.spec.name
                  << " -> cell " << grid.formatIndex(humidifier.anchorCell)
                  << ", mode " << humidifier.spec.mode
                  << ", affected cells " << humidifier.influenceCells.size() << "\n";
    }
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
        const std::vector<greenhouse::MappedPlantPoint> plants =
            greenhouse::mapPlantsToGrid(config.plants, grid);
        const std::vector<greenhouse::MappedVent> vents =
            greenhouse::mapVentsToGrid(config.vents, grid);
        const std::vector<greenhouse::MappedHeater> heaters =
            greenhouse::mapHeatersToGrid(config.heaters, grid);
        const std::vector<greenhouse::MappedHumidifier> humidifiers =
            greenhouse::mapHumidifiersToGrid(config.humidifiers, grid);

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
        printPlantMapping(plants, grid);
        printDeviceMapping(vents, heaters, humidifiers, grid);
        std::cout << "\nDay 2 geometry layer is ready. Physics modules will be added next.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Startup error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
