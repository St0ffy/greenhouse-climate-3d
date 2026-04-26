#pragma once

#include "Types.h"

#include <string>

namespace greenhouse {

struct MaterialProperties {
    std::string name = "polycarbonate";
    double heatLossCoefficient = 0.35;
    double solarTransmission = 0.75;
};

MaterialProperties makeMaterial(const MaterialSpec& spec);
MaterialSpec presetMaterialSpec(const std::string& name);
std::string normalizeMaterialName(const std::string& name);

} // namespace greenhouse
