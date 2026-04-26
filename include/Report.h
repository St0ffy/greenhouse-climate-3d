#pragma once

#include "Devices.h"
#include "Geometry.h"
#include "Optimizer.h"
#include "Simulator.h"
#include "Types.h"

#include <string>

namespace greenhouse {

std::string buildSimulationReport(
    const SimulationResult& result,
    const Grid3D& grid,
    const MappedDeviceSet& devices
);

std::string buildOptimizationReport(
    const OptimizationResult& result,
    const Grid3D& grid
);

std::string writeReportFile(
    const std::string& reportText,
    const OutputSpec& output
);

} // namespace greenhouse
