#pragma once

#include "Devices.h"
#include "Geometry.h"
#include "Simulator.h"

namespace greenhouse {

void replaySimulationInTerminal(
    const SimulationResult& result,
    const Grid3D& grid,
    const MappedDeviceSet& devices
);

} // namespace greenhouse
