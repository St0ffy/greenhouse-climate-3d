#include "Material.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace greenhouse {

std::string normalizeMaterialName(const std::string& name) {
    std::string normalized = name;
    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        }
    );

    normalized.erase(
        std::remove_if(
            normalized.begin(),
            normalized.end(),
            [](unsigned char ch) {
                return std::isspace(ch) != 0;
            }
        ),
        normalized.end()
    );

    return normalized;
}

MaterialSpec presetMaterialSpec(const std::string& name) {
    const std::string normalized = normalizeMaterialName(name);

    if (normalized == "glass") {
        return {"glass", 0.55, 0.90};
    }

    if (normalized == "film" || normalized == "plasticfilm") {
        return {"film", 0.70, 0.80};
    }

    if (normalized == "polycarbonate" || normalized == "polycarbonat") {
        return {"polycarbonate", 0.35, 0.75};
    }

    return {normalized.empty() ? "custom" : normalized, 0.45, 0.70};
}

MaterialProperties makeMaterial(const MaterialSpec& spec) {
    MaterialSpec base = presetMaterialSpec(spec.name);

    if (spec.heatLossCoefficient >= 0.0) {
        base.heatLossCoefficient = spec.heatLossCoefficient;
    }

    if (spec.solarTransmission >= 0.0) {
        base.solarTransmission = spec.solarTransmission;
    }

    if (base.heatLossCoefficient < 0.0) {
        throw std::invalid_argument("material heat loss coefficient cannot be negative");
    }

    if (base.solarTransmission < 0.0 || base.solarTransmission > 1.0) {
        throw std::invalid_argument("material solar transmission must be in range 0..1");
    }

    return {
        base.name,
        base.heatLossCoefficient,
        base.solarTransmission
    };
}

} // namespace greenhouse
