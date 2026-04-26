#include "Config.h"

#include <fstream>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

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

std::string readObject(const std::string& text, const std::string& key) {
    const std::string quotedKey = "\"" + key + "\"";
    const std::size_t keyPosition = text.find(quotedKey);
    if (keyPosition == std::string::npos) {
        return {};
    }

    const std::size_t objectStart = text.find('{', keyPosition + quotedKey.size());
    if (objectStart == std::string::npos) {
        return {};
    }

    int braceDepth = 0;
    for (std::size_t i = objectStart; i < text.size(); ++i) {
        if (text[i] == '{') {
            ++braceDepth;
        } else if (text[i] == '}') {
            --braceDepth;
            if (braceDepth == 0) {
                return text.substr(objectStart, i - objectStart + 1);
            }
        }
    }

    return {};
}

Vec3 readVec3(const std::string& text, const std::string& key, const Vec3& fallback) {
    const std::regex pattern(
        "\"" + key + "\"\\s*:\\s*\\[\\s*"
        "(-?[0-9]+(?:\\.[0-9]+)?)\\s*,\\s*"
        "(-?[0-9]+(?:\\.[0-9]+)?)\\s*,\\s*"
        "(-?[0-9]+(?:\\.[0-9]+)?)\\s*\\]"
    );
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        return {
            std::stod(match[1].str()),
            std::stod(match[2].str()),
            std::stod(match[3].str())
        };
    }
    return fallback;
}

std::vector<std::string> readObjectArray(const std::string& text, const std::string& key) {
    const std::string quotedKey = "\"" + key + "\"";
    const std::size_t keyPosition = text.find(quotedKey);
    if (keyPosition == std::string::npos) {
        return {};
    }

    const std::size_t arrayStart = text.find('[', keyPosition + quotedKey.size());
    if (arrayStart == std::string::npos) {
        return {};
    }

    int bracketDepth = 0;
    std::size_t arrayEnd = std::string::npos;
    for (std::size_t i = arrayStart; i < text.size(); ++i) {
        if (text[i] == '[') {
            ++bracketDepth;
        } else if (text[i] == ']') {
            --bracketDepth;
            if (bracketDepth == 0) {
                arrayEnd = i;
                break;
            }
        }
    }

    if (arrayEnd == std::string::npos) {
        return {};
    }

    const std::string arrayText = text.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    std::vector<std::string> objects;
    int braceDepth = 0;
    std::size_t objectStart = std::string::npos;
    for (std::size_t i = 0; i < arrayText.size(); ++i) {
        if (arrayText[i] == '{') {
            if (braceDepth == 0) {
                objectStart = i;
            }
            ++braceDepth;
        } else if (arrayText[i] == '}') {
            --braceDepth;
            if (braceDepth == 0 && objectStart != std::string::npos) {
                objects.push_back(arrayText.substr(objectStart, i - objectStart + 1));
                objectStart = std::string::npos;
            }
        }
    }

    return objects;
}

std::vector<VentSpec> readVents(const std::string& text) {
    std::vector<VentSpec> vents;
    for (const std::string& object : readObjectArray(text, "vents")) {
        vents.push_back({
            readString(object, "name", "vent"),
            readVec3(object, "position", {0.0, 0.0, 0.0}),
            readNumber(object, "opening", 0.0),
            readNumber(object, "influence_radius_m", 1.0)
        });
    }
    return vents;
}

std::vector<HeaterSpec> readHeaters(const std::string& text) {
    std::vector<HeaterSpec> heaters;
    for (const std::string& object : readObjectArray(text, "heaters")) {
        heaters.push_back({
            readString(object, "name", "heater"),
            readVec3(object, "position", {0.0, 0.0, 0.0}),
            readNumber(object, "power_w", 1000.0),
            readNumber(object, "influence_radius_m", 1.0)
        });
    }
    return heaters;
}

std::vector<HumidifierSpec> readHumidifiers(const std::string& text) {
    std::vector<HumidifierSpec> humidifiers;
    for (const std::string& object : readObjectArray(text, "humidifiers")) {
        humidifiers.push_back({
            readString(object, "name", "humidifier"),
            readVec3(object, "position", {0.0, 0.0, 0.0}),
            readString(object, "mode", "off"),
            readNumber(object, "influence_radius_m", 1.0)
        });
    }
    return humidifiers;
}

std::vector<PlantPoint> readPlants(const std::string& text) {
    std::vector<PlantPoint> plants;
    for (const std::string& object : readObjectArray(text, "plants")) {
        plants.push_back({
            readString(object, "name", "plant"),
            readVec3(object, "position", {0.0, 0.0, 0.0}),
            readNumber(object, "target_temperature_c", 22.0)
        });
    }
    return plants;
}

} // namespace

SimulationConfig loadConfig(const std::string& path) {
    const std::string text = readFile(path);
    const std::string runSection = readObject(text, "run");
    const std::string greenhouseSection = readObject(text, "greenhouse");
    const std::string gridSection = readObject(text, "grid");
    const std::string initialStateSection = readObject(text, "initial_state");
    const std::string weatherSection = readObject(text, "weather");
    const std::string materialSection = readObject(text, "material");
    const std::string humiditySection = readObject(text, "humidity");
    SimulationConfig config;

    config.mode = readString(runSection, "mode", "simulate");
    config.greenhouseSize = {
        readNumber(greenhouseSection, "length_m", 12.0),
        readNumber(greenhouseSection, "width_m", 6.0),
        readNumber(greenhouseSection, "height_m", 3.0)
    };
    config.gridSize = {
        readInt(gridSection, "nx", 12),
        readInt(gridSection, "ny", 6),
        readInt(gridSection, "nz", 4)
    };
    config.durationSeconds = readNumber(runSection, "duration_seconds", 6.0 * 3600.0);
    config.timeStepSeconds = readNumber(runSection, "time_step_seconds", 60.0);
    config.initialTemperatureC = readNumber(initialStateSection, "temperature_c", 18.0);
    config.initialHumidityPercent = readNumber(initialStateSection, "humidity_percent", 60.0);
    config.weather = {
        readNumber(weatherSection, "outside_temperature_c", 5.0),
        readNumber(weatherSection, "outside_humidity_percent", 75.0),
        readNumber(weatherSection, "solar_radiation_w_m2", 350.0),
        readString(weatherSection, "weather_file", "")
    };
    config.material = {
        readString(materialSection, "name", "polycarbonate"),
        readNumber(materialSection, "heat_loss_coefficient", -1.0),
        readNumber(materialSection, "solar_transmission", -1.0)
    };
    config.humidityEnabled = readBool(humiditySection, "enabled", true);
    config.humidifierMode = readString(humiditySection, "humidifier_mode", "medium");
    config.heaterPowerW = readNumber(text, "power_w", 1200.0);
    config.plants = {
        {"plant_1", {3.0, 2.0, 0.5}, 22.0},
        {"plant_2", {6.0, 3.0, 0.5}, 22.0},
        {"plant_3", {9.0, 4.0, 0.5}, 22.0}
    };
    config.vents = {
        {"left_roof_vent", {2.0, 0.5, 2.8}, 0.3, 2.0},
        {"right_roof_vent", {10.0, 5.5, 2.8}, 0.3, 2.0}
    };
    config.heaters = {
        {"heater_1", {3.0, 3.0, 0.2}, 1200.0, 2.5},
        {"heater_2", {9.0, 3.0, 0.2}, 1200.0, 2.5}
    };
    config.humidifiers = {
        {"humidifier_1", {6.0, 3.0, 0.5}, "medium", 2.0}
    };

    const std::vector<VentSpec> parsedVents = readVents(text);
    if (!parsedVents.empty()) {
        config.vents = parsedVents;
    }

    const std::vector<HeaterSpec> parsedHeaters = readHeaters(text);
    if (!parsedHeaters.empty()) {
        config.heaters = parsedHeaters;
        config.heaterPowerW = parsedHeaters.front().powerW;
    }

    const std::vector<HumidifierSpec> parsedHumidifiers = readHumidifiers(text);
    if (!parsedHumidifiers.empty()) {
        config.humidifiers = parsedHumidifiers;
    }

    const std::vector<PlantPoint> parsedPlants = readPlants(text);
    if (!parsedPlants.empty()) {
        config.plants = parsedPlants;
    }

    return config;
}

} // namespace greenhouse
