#include "Report.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace greenhouse {

namespace {

std::string formatBool(bool value) {
    return value ? "true" : "false";
}

std::filesystem::path ensureOutputDirectory(const OutputSpec& output) {
    const std::filesystem::path directory =
        output.directory.empty() ? std::filesystem::path("outputs") : output.directory;
    std::filesystem::create_directories(directory);
    return directory;
}

} // namespace

std::string buildSimulationReport(
    const SimulationResult& result,
    const Grid3D& grid,
    const MappedDeviceSet& devices
) {
    std::ostringstream report;
    report << std::fixed << std::setprecision(2);

    report << "Greenhouse Climate 3D Report\n";
    report << "============================\n";
    report << "Mode: " << result.config.mode << "\n";
    report << "Duration, seconds: " << result.durationSeconds << "\n";
    report << "Time step, seconds: " << result.timeStepSeconds << "\n";
    report << "Frames: " << result.frames.size() << "\n";
    report << "Grid: "
           << grid.gridSize().nx << " x "
           << grid.gridSize().ny << " x "
           << grid.gridSize().nz
           << " (" << grid.cellCount() << " cells)\n";
    report << "Humidity enabled: " << formatBool(result.config.humidityEnabled) << "\n";
    report << "Material: " << result.material.name
           << " (heat loss " << result.material.heatLossCoefficient
           << ", solar transmission " << result.material.solarTransmission << ")\n";
    report << "Devices: "
           << devices.vents.size() << " vents, "
           << devices.heaters.size() << " heaters, "
           << devices.humidifiers.size() << " humidifiers, "
           << devices.plants.size() << " plant points\n";
    report << "Total heater power, W: " << devices.totalHeaterPowerW << "\n";
    report << "Total heater energy, kWh: " << result.totalHeaterEnergyKWh << "\n";

    if (!result.frames.empty()) {
        const SimulationFrame& initial = result.frames.front();
        const SimulationFrame& final = result.frames.back();

        report << "\nInitial climate\n";
        report << "---------------\n";
        report << "Average temperature, C: "
               << initial.climate.temperatureStep.temperature.averageTemperatureC << "\n";
        report << "Average humidity, %: "
               << initial.climate.humidity.averageHumidityPercent << "\n";
        report << "Plant average temperature, C: "
               << initial.plantTemperature.averageTemperatureC << "\n";

        report << "\nFinal climate\n";
        report << "-------------\n";
        report << "Average temperature, C: "
               << final.climate.temperatureStep.temperature.averageTemperatureC << "\n";
        report << "Min temperature, C: "
               << final.climate.temperatureStep.temperature.minTemperatureC << "\n";
        report << "Max temperature, C: "
               << final.climate.temperatureStep.temperature.maxTemperatureC << "\n";
        report << "Average humidity, %: "
               << final.climate.humidity.averageHumidityPercent << "\n";
        report << "Min humidity, %: "
               << final.climate.humidity.minHumidityPercent << "\n";
        report << "Max humidity, %: "
               << final.climate.humidity.maxHumidityPercent << "\n";
        report << "Plant average temperature, C: "
               << final.plantTemperature.averageTemperatureC << "\n";
        report << "Plant target temperature, C: "
               << final.plantTemperature.averageTargetTemperatureC << "\n";
        report << "Plant average temperature error, C: "
               << final.plantTemperature.averageAbsoluteErrorC << "\n";
        report << "Plant max temperature error, C: "
               << final.plantTemperature.maxAbsoluteErrorC << "\n";
        report << "Plant average humidity, %: "
               << final.plantHumidity.averageHumidityPercent << "\n";

        report << "\nInterpretation\n";
        report << "--------------\n";
        const double signedAverageDifference =
            final.plantTemperature.averageTemperatureC
            - final.plantTemperature.averageTargetTemperatureC;
        if (final.plantTemperature.averageAbsoluteErrorC <= 1.0) {
            report << "Plant zones are close to the target temperature.\n";
        } else if (std::fabs(signedAverageDifference) <= 1.0) {
            report << "Plant average temperature is near target, but local plant cells are uneven. Optimizer should improve heater placement balance across plant zones.\n";
        } else if (signedAverageDifference < 0.0) {
            report << "Plant zones are colder than target on average. Optimizer should try moving heaters closer to plant cells or increasing heater coverage.\n";
        } else {
            report << "Plant zones are warmer than target on average. Optimizer should try moving heaters away from plant cells or reducing heater coverage.\n";
        }
    }

    return report.str();
}

std::string buildOptimizationReport(
    const OptimizationResult& result,
    const Grid3D& grid
) {
    std::ostringstream report;
    report << std::fixed << std::setprecision(2);

    report << "Greenhouse Climate 3D Optimization Report\n";
    report << "=========================================\n";
    report << "Candidate positions: " << result.candidatePositions.size() << "\n";
    report << "Tested heater layouts: " << result.testedLayouts << "\n";
    report << "Improved: " << formatBool(result.improved) << "\n";
    report << "Baseline quality: " << result.baselineScore.quality << "\n";
    report << "Best quality: " << result.bestScore.quality << "\n";
    report << "Baseline plant avg error, C: "
           << result.baselineScore.averagePlantTemperatureErrorC << "\n";
    report << "Best plant avg error, C: "
           << result.bestScore.averagePlantTemperatureErrorC << "\n";
    report << "Baseline plant max error, C: "
           << result.baselineScore.maxPlantTemperatureErrorC << "\n";
    report << "Best plant max error, C: "
           << result.bestScore.maxPlantTemperatureErrorC << "\n";
    report << "Best heater energy, kWh: " << result.bestScore.energyKWh << "\n";
    report << "Recommendation: " << result.recommendation << "\n";

    report << "\nRecommended heater positions\n";
    report << "----------------------------\n";
    for (const MappedHeater& heater : result.bestDevices.heaters) {
        report << heater.spec.name
               << ": x=" << heater.spec.position.x
               << ", y=" << heater.spec.position.y
               << ", z=" << heater.spec.position.z
               << ", power_w=" << heater.spec.powerW
               << ", cell=" << grid.formatIndex(heater.anchorCell)
               << "\n";
    }

    report << "\nBest simulation details\n";
    report << "-----------------------\n";
    report << buildSimulationReport(
        result.bestSimulation,
        grid,
        result.bestDevices
    );

    return report.str();
}

std::string writeReportFile(
    const std::string& reportText,
    const OutputSpec& output
) {
    if (!output.writeReport) {
        return {};
    }

    const std::filesystem::path reportPath =
        ensureOutputDirectory(output) / "report.txt";
    std::ofstream file(reportPath);
    if (!file) {
        throw std::runtime_error("cannot open report file: " + reportPath.string());
    }

    file << reportText;
    return reportPath.string();
}

} // namespace greenhouse
