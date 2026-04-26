#pragma once

#include "Devices.h"
#include "Geometry.h"
#include "Simulator.h"
#include "Types.h"

#include <string>

namespace greenhouse {

std::string buildSimulationReport(
    const SimulationResult& result,
    const Grid3D& grid,
    const MappedDeviceSet& devices
);

std::string writeReportFile(
    const std::string& reportText,
    const OutputSpec& output
);

} // namespace greenhouse
