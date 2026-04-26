#include "Devices.h"
#include "Material.h"
#include "Physics.h"
#include "Weather.h"

#include <cassert>
#include <cmath>
#include <vector>

namespace {

bool almostEqual(double left, double right) {
    return std::fabs(left - right) < 1e-9;
}

greenhouse::MappedDeviceSet emptyDevicesFor(const greenhouse::Grid3D& grid) {
    const std::vector<greenhouse::PlantPoint> plants = {
        {"plant", {0.5, 0.5, 0.5}, 22.0}
    };

    return greenhouse::mapDeviceSetToGrid(
        plants,
        {},
        {},
        {},
        grid
    );
}

} // namespace

int main() {
    const greenhouse::WeatherCondition noSun{0.0, 0.0, 50.0, 0.0};
    const greenhouse::MaterialProperties noLoss{"test", 0.0, 0.0};

    {
        const greenhouse::Grid3D grid({3.0, 1.0, 1.0}, {3, 1, 1});
        const greenhouse::MappedDeviceSet devices = emptyDevicesFor(grid);
        std::vector<greenhouse::CellState> cells = {
            {10.0, 60.0},
            {30.0, 60.0},
            {10.0, 60.0}
        };

        greenhouse::TemperaturePhysicsSettings settings;
        settings.heatTransferRatePerSecond = 0.1;
        settings.boundaryHeatLossRatePerSecond = 0.0;
        settings.solarGainCPerWm2Second = 0.0;
        settings.heaterGainCPerWattSecond = 0.0;

        const greenhouse::TemperatureStepResult result =
            greenhouse::advanceTemperature(
                cells,
                grid,
                noSun,
                noLoss,
                devices,
                1.0,
                settings
            );

        assert(result.cells[0].temperatureC > 10.0);
        assert(result.cells[1].temperatureC < 30.0);
        assert(result.cells[2].temperatureC > 10.0);
    }

    {
        const greenhouse::Grid3D grid({3.0, 3.0, 1.0}, {3, 3, 1});
        const std::vector<greenhouse::PlantPoint> plants = {
            {"plant", {1.5, 1.5, 0.5}, 22.0}
        };
        const std::vector<greenhouse::HeaterSpec> heaters = {
            {"heater", {1.5, 1.5, 0.5}, 100.0, 0.0}
        };
        const greenhouse::MappedDeviceSet devices =
            greenhouse::mapDeviceSetToGrid(plants, {}, heaters, {}, grid);
        const std::vector<greenhouse::CellState> cells =
            greenhouse::makeInitialCells(grid, 20.0, 60.0);

        greenhouse::TemperaturePhysicsSettings settings;
        settings.heatTransferRatePerSecond = 0.0;
        settings.boundaryHeatLossRatePerSecond = 0.0;
        settings.solarGainCPerWm2Second = 0.0;
        settings.heaterGainCPerWattSecond = 0.001;

        const greenhouse::TemperatureStepResult result =
            greenhouse::advanceTemperature(
                cells,
                grid,
                noSun,
                noLoss,
                devices,
                10.0,
                settings
            );

        const std::size_t center = grid.linearIndex({1, 1, 0});
        assert(result.cells[center].temperatureC > 20.0);
        assert(almostEqual(result.summary.heaterEnergyKWh, 100.0 * 10.0 / 3'600'000.0));
    }

    {
        const greenhouse::Grid3D grid({1.0, 1.0, 2.0}, {1, 1, 2});
        const greenhouse::MappedDeviceSet devices = emptyDevicesFor(grid);
        const std::vector<greenhouse::CellState> cells =
            greenhouse::makeInitialCells(grid, 20.0, 60.0);
        const greenhouse::WeatherCondition sun{0.0, 0.0, 50.0, 1000.0};
        const greenhouse::MaterialProperties transparent{"test", 0.0, 1.0};

        greenhouse::TemperaturePhysicsSettings settings;
        settings.heatTransferRatePerSecond = 0.0;
        settings.boundaryHeatLossRatePerSecond = 0.0;
        settings.solarGainCPerWm2Second = 0.001;
        settings.heaterGainCPerWattSecond = 0.0;

        const greenhouse::TemperatureStepResult result =
            greenhouse::advanceTemperature(
                cells,
                grid,
                sun,
                transparent,
                devices,
                10.0,
                settings
            );

        const double lower = result.cells[grid.linearIndex({0, 0, 0})].temperatureC;
        const double upper = result.cells[grid.linearIndex({0, 0, 1})].temperatureC;
        assert(upper > lower);
    }

    {
        const greenhouse::Grid3D grid({1.0, 1.0, 1.0}, {1, 1, 1});
        const greenhouse::MappedDeviceSet devices = emptyDevicesFor(grid);
        const std::vector<greenhouse::CellState> cells =
            greenhouse::makeInitialCells(grid, 20.0, 60.0);
        const greenhouse::WeatherCondition coldOutside{0.0, 0.0, 50.0, 0.0};
        const greenhouse::MaterialProperties lossy{"test", 1.0, 0.0};

        greenhouse::TemperaturePhysicsSettings settings;
        settings.heatTransferRatePerSecond = 0.0;
        settings.boundaryHeatLossRatePerSecond = 0.1;
        settings.solarGainCPerWm2Second = 0.0;
        settings.heaterGainCPerWattSecond = 0.0;

        const greenhouse::TemperatureStepResult result =
            greenhouse::advanceTemperature(
                cells,
                grid,
                coldOutside,
                lossy,
                devices,
                1.0,
                settings
            );

        assert(result.cells.front().temperatureC < 20.0);
    }

    return 0;
}
