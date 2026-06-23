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

std::string formatTargetTime(const ControlComparisonSummary& summary) {
    if (!summary.targetReached) {
        return "not reached";
    }

    std::ostringstream output;
    output << std::fixed << std::setprecision(2)
           << summary.targetReachedSeconds << " s";
    return output.str();
}

std::filesystem::path ensureOutputDirectory(const OutputSpec& output) {
    const std::filesystem::path directory =
        output.directory.empty()
            ? std::filesystem::path("outputs")
            : std::filesystem::path(output.directory);
    std::filesystem::create_directories(directory);
    return directory;
}

void appendComparisonRun(
    std::ostringstream& report,
    const std::string& title,
    const ControlComparisonRun& run,
    double quality
) {
    const ControlComparisonSummary& summary = run.summary;
    report << "\n" << title << "\n";
    report << std::string(title.size(), '-') << "\n";
    report << "Mode label: " << run.result.config.mode << "\n";
    report << "Controller strategy: " << run.result.config.control.strategy << "\n";
    report << "ML enabled: " << formatBool(run.result.config.control.mlEnabled) << "\n";
    report << "Frames: " << run.result.frames.size() << "\n";
    report << "Target reached: " << formatBool(summary.targetReached)
           << " (" << formatTargetTime(summary) << ")\n";
    report << "Final plant avg temperature error, C: "
           << summary.finalAverageTemperatureErrorC << "\n";
    report << "Final plant max temperature error, C: "
           << summary.finalMaxTemperatureErrorC << "\n";
    report << "Final plant avg humidity error, %: "
           << summary.finalAverageHumidityErrorPercent << "\n";
    report << "Final plant max humidity error, %: "
           << summary.finalMaxHumidityErrorPercent << "\n";
    report << "Final plant avg comfort: "
           << summary.finalAverageComfort << "\n";
    report << "Final plant avg health: "
           << summary.finalAverageHealth << "\n";
    report << "Final plant min health: "
           << summary.finalMinHealth << "\n";
    report << "Final alive plants: "
           << summary.finalAlivePlants << "\n";
    report << "Total heater energy, kWh: "
           << summary.totalHeaterEnergyKWh << "\n";
    report << "Total device energy, kWh: "
           << summary.totalDeviceEnergyKWh << "\n";
    report << "Comparison quality score: " << quality << "\n";
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
    report << "Total device energy, kWh: " << result.totalDeviceEnergyKWh << "\n";
    report << "Controller enabled: " << formatBool(result.config.control.enabled)
           << ", ML enabled: " << formatBool(result.config.control.mlEnabled)
           << ", strategy: " << result.config.control.strategy << "\n";

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
        report << "Plant average health: "
               << initial.plantGrowth.averageHealth << "\n";

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
        report << "Average plant humidity uptake, % per cell: "
               << final.climate.averagePlantHumidityUptakePercent << "\n";
        report << "Plant average health: "
               << final.plantGrowth.averageHealth << "\n";
        report << "Plant min health: "
               << final.plantGrowth.minHealth << "\n";
        report << "Plant average growth: "
               << final.plantGrowth.averageGrowth << "\n";
        report << "Plant average comfort: "
               << final.plantGrowth.averageComfort << "\n";
        report << "Plant alive count: "
               << final.plantGrowth.aliveCount << "\n";
        report << "Controller reward: "
               << final.control.reward << "\n";
        report << "Failed devices: "
               << final.control.failedDeviceCount << "\n";

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

std::string buildControlComparisonReport(
    const ControlComparisonResult& result,
    const Grid3D& grid
) {
    std::ostringstream report;
    report << std::fixed << std::setprecision(2);

    report << "Greenhouse Climate 3D Control Comparison Report\n";
    report << "===============================================\n";
    report << "Grid: "
           << grid.gridSize().nx << " x "
           << grid.gridSize().ny << " x "
           << grid.gridSize().nz
           << " (" << grid.cellCount() << " cells)\n";
    report << "Temperature tolerance, C: "
           << result.onOff.result.config.control.temperatureToleranceC << "\n";
    report << "Humidity tolerance, %: "
           << result.onOff.result.config.control.humidityTolerancePercent << "\n";

    appendComparisonRun(report, "On/off sensor automation", result.onOff, result.onOffQuality);
    appendComparisonRun(report, "ML adaptive control", result.ml, result.mlQuality);

    report << "\nRecommendation\n";
    report << "--------------\n";
    report << result.recommendation << "\n";

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

std::string writeComparisonReportFile(
    const std::string& reportText,
    const OutputSpec& output
) {
    if (!output.writeReport) {
        return {};
    }

    const std::filesystem::path reportPath =
        ensureOutputDirectory(output) / "comparison_report.txt";
    std::ofstream file(reportPath);
    if (!file) {
        throw std::runtime_error("cannot open report file: " + reportPath.string());
    }

    file << reportText;
    return reportPath.string();
}

} // namespace greenhouse
