#pragma once

#include "Devices.h"
#include "Geometry.h"
#include "Material.h"
#include "Simulator.h"
#include "Weather.h"

namespace greenhouse {

void replaySimulationInTerminal(
    const SimulationResult& result,
    const Grid3D& grid,
    const MappedDeviceSet& devices
);

SimulationResult runInteractiveSimulationInTerminal(
    const SimulationConfig& config,
    const Grid3D& grid,
    const WeatherTimeline& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices,
    const ClimatePhysicsSettings& settings = ClimatePhysicsSettings{}
);

} // namespace greenhouse
