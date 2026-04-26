#include "Config.h"

#include <cassert>
#include <cmath>

namespace {

bool almostEqual(double left, double right) {
    return std::fabs(left - right) < 1e-9;
}

} // namespace

int main() {
    const greenhouse::SimulationConfig config =
        greenhouse::loadConfig("configs/default_config.json");

    assert(config.mode == "simulate");
    assert(almostEqual(config.greenhouseSize.length, 12.0));
    assert(almostEqual(config.greenhouseSize.width, 6.0));
    assert(almostEqual(config.greenhouseSize.height, 3.0));
    assert(config.gridSize.nx == 12);
    assert(config.gridSize.ny == 6);
    assert(config.gridSize.nz == 4);
    assert(almostEqual(config.durationSeconds, 21600.0));
    assert(almostEqual(config.timeStepSeconds, 60.0));

    assert(almostEqual(config.weather.outsideTemperatureC, 5.0));
    assert(almostEqual(config.weather.outsideHumidityPercent, 75.0));
    assert(almostEqual(config.weather.solarRadiationWm2, 350.0));
    assert(config.weather.weatherFile == "data/weather_sample.csv");

    assert(config.material.name == "polycarbonate");
    assert(almostEqual(config.material.heatLossCoefficient, 0.35));
    assert(almostEqual(config.material.solarTransmission, 0.75));

    assert(config.vents.size() == 2);
    assert(config.heaters.size() == 2);
    assert(config.humidifiers.size() == 1);
    assert(config.plants.size() == 3);

    return 0;
}
