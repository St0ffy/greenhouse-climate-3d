#include "Material.h"

#include <cassert>
#include <cmath>
#include <stdexcept>

namespace {

bool almostEqual(double left, double right) {
    return std::fabs(left - right) < 1e-9;
}

} // namespace

int main() {
    const greenhouse::MaterialProperties glass =
        greenhouse::makeMaterial({"glass", -1.0, -1.0});
    assert(glass.name == "glass");
    assert(almostEqual(glass.heatLossCoefficient, 0.55));
    assert(almostEqual(glass.solarTransmission, 0.90));

    const greenhouse::MaterialProperties custom =
        greenhouse::makeMaterial({"custom_cover", 0.42, 0.66});
    assert(custom.name == "custom_cover");
    assert(almostEqual(custom.heatLossCoefficient, 0.42));
    assert(almostEqual(custom.solarTransmission, 0.66));

    bool failed = false;
    try {
        greenhouse::makeMaterial({"bad", 0.40, 2.0});
    } catch (const std::invalid_argument&) {
        failed = true;
    }
    assert(failed);

    return 0;
}
