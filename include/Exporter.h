#pragma once

#include "Geometry.h"
#include "Simulator.h"
#include "Types.h"

#include <string>
#include <vector>

namespace greenhouse {

struct ExportedFiles {
    std::vector<std::string> paths;
    std::string cellHistoryCsvPath;
    std::string metricsCsvPath;
    std::string summaryJsonPath;
};

ExportedFiles exportSimulationResult(
    const SimulationResult& result,
    const Grid3D& grid,
    const OutputSpec& output
);

} // namespace greenhouse
