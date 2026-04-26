#include "Config.h"

#include <fstream>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <string>

namespace greenhouse {

namespace {

std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open config file: " + path);
    }

    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    );
}

double readNumber(const std::string& text, const std::string& key, double fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        return std::stod(match[1].str());
    }
    return fallback;
}

int readInt(const std::string& text, const std::string& key, int fallback) {
    return static_cast<int>(readNumber(text, key, fallback));
}

bool readBool(const std::string& text, const std::string& key, bool fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        return match[1].str() == "true";
    }
    return fallback;
}

std::string readString(const std::string& text, const std::string& key, const std::string& fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        return match[1].str();
    }
    return fallback;
}

} // namespace

SimulationConfig loadConfig(const std::string& path) {
    const std::string text = readFile(path);
    SimulationConfig config;

    config.mode = readString(text, "mode", "simulate");
    config.greenhouseSize = {
        readNumber(text, "length_m", 12.0),
        readNumber(text, "width_m", 6.0),
        readNumber(text, "height_m", 3.0)
    };
    config.gridSize = {
        readInt(text, "nx", 12),
        readInt(text, "ny", 6),
        readInt(text, "nz", 4)
    };
    config.durationSeconds = readNumber(text, "duration_seconds", 6.0 * 3600.0);
    config.timeStepSeconds = readNumber(text, "time_step_seconds", 60.0);
    config.initialTemperatureC = readNumber(text, "temperature_c", 18.0);
    config.initialHumidityPercent = readNumber(text, "humidity_percent", 60.0);
    config.outsideTemperatureC = readNumber(text, "outside_temperature_c", 5.0);
    config.outsideHumidityPercent = readNumber(text, "outside_humidity_percent", 75.0);
    config.solarRadiationWm2 = readNumber(text, "solar_radiation_w_m2", 350.0);
    config.humidityEnabled = readBool(text, "enabled", true);
    config.humidifierMode = readString(text, "humidifier_mode", "medium");
    config.heaterPowerW = readNumber(text, "power_w", 1200.0);
    config.plants = {
        {"plant_1", {3.0, 2.0, 0.5}, 22.0},
        {"plant_2", {6.0, 3.0, 0.5}, 22.0},
        {"plant_3", {9.0, 4.0, 0.5}, 22.0}
    };

    return config;
}

} // namespace greenhouse
