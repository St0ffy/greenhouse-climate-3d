#include "Config.h"

#include <exception>
#include <iostream>
#include <string>

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

        std::cout << "Greenhouse Climate 3D\n";
        std::cout << "Mode: " << config.mode << "\n";
        std::cout << "Config: " << configPath << "\n";
        std::cout << "Grid: "
                  << config.gridSize.nx << " x "
                  << config.gridSize.ny << " x "
                  << config.gridSize.nz << "\n";
        std::cout << "Duration, seconds: " << config.durationSeconds << "\n";
        std::cout << "Time step, seconds: " << config.timeStepSeconds << "\n";
        std::cout << "\nDay 1 scaffold is ready. Simulation modules will be added next.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Startup error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}

