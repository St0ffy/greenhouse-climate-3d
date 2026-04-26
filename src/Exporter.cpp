#include "Exporter.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace greenhouse {

namespace {

std::filesystem::path ensureOutputDirectory(const OutputSpec& output) {
    const std::filesystem::path directory =
        output.directory.empty() ? std::filesystem::path("outputs") : output.directory;

    std::filesystem::create_directories(directory);
    return directory;
}

std::ofstream openOutputFile(const std::filesystem::path& path) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("cannot open output file: " + path.string());
    }

    file << std::fixed << std::setprecision(4);
    return file;
}

std::string jsonEscape(const std::string& value) {
    std::ostringstream output;
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                output << "\\\\";
                break;
            case '"':
                output << "\\\"";
                break;
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\t':
                output << "\\t";
                break;
            default:
                output << ch;
                break;
        }
    }
    return output.str();
}

void writeCellHistoryCsv(
    const SimulationResult& result,
    const Grid3D& grid,
    const std::filesystem::path& path
) {
    std::ofstream file = openOutputFile(path);
    file << "time_seconds,linear_index,x,y,z,temperature_c,humidity_percent\n";

    for (const SimulationFrame& frame : result.frames) {
        for (std::size_t linear = 0; linear < frame.cells.size(); ++linear) {
            const GridIndex index = grid.indexFromLinear(linear);
            const CellState& cell = frame.cells[linear];
            file << frame.timeSeconds << ','
                 << linear << ','
                 << index.x << ','
                 << index.y << ','
                 << index.z << ','
                 << cell.temperatureC << ','
                 << cell.humidityPercent << '\n';
        }
    }
}

void writeMetricsCsv(
    const SimulationResult& result,
    const std::filesystem::path& path
) {
    std::ofstream file = openOutputFile(path);
    file
        << "time_seconds,"
        << "outside_temperature_c,outside_humidity_percent,solar_radiation_w_m2,"
        << "avg_temperature_c,min_temperature_c,max_temperature_c,"
        << "avg_humidity_percent,min_humidity_percent,max_humidity_percent,"
        << "plant_avg_temperature_c,plant_target_temperature_c,plant_avg_temperature_error_c,"
        << "plant_avg_humidity_percent,"
        << "cumulative_heater_energy_kwh,"
        << "avg_vent_temperature_delta_c,avg_vent_humidity_delta_percent\n";

    for (const SimulationFrame& frame : result.frames) {
        file << frame.timeSeconds << ','
             << frame.weather.outsideTemperatureC << ','
             << frame.weather.outsideHumidityPercent << ','
             << frame.weather.solarRadiationWm2 << ','
             << frame.climate.temperatureStep.temperature.averageTemperatureC << ','
             << frame.climate.temperatureStep.temperature.minTemperatureC << ','
             << frame.climate.temperatureStep.temperature.maxTemperatureC << ','
             << frame.climate.humidity.averageHumidityPercent << ','
             << frame.climate.humidity.minHumidityPercent << ','
             << frame.climate.humidity.maxHumidityPercent << ','
             << frame.plantTemperature.averageTemperatureC << ','
             << frame.plantTemperature.averageTargetTemperatureC << ','
             << frame.plantTemperature.averageAbsoluteErrorC << ','
             << frame.plantHumidity.averageHumidityPercent << ','
             << frame.cumulativeHeaterEnergyKWh << ','
             << frame.climate.averageVentTemperatureDeltaC << ','
             << frame.climate.averageVentHumidityDeltaPercent << '\n';
    }
}

void writeSummaryJson(
    const SimulationResult& result,
    const Grid3D& grid,
    const std::filesystem::path& path
) {
    std::ofstream file = openOutputFile(path);
    const SimulationFrame* finalFrame =
        result.frames.empty() ? nullptr : &result.frames.back();

    file << "{\n";
    file << "  \"mode\": \"" << jsonEscape(result.config.mode) << "\",\n";
    file << "  \"duration_seconds\": " << result.durationSeconds << ",\n";
    file << "  \"time_step_seconds\": " << result.timeStepSeconds << ",\n";
    file << "  \"frame_count\": " << result.frames.size() << ",\n";
    file << "  \"grid\": {\n";
    file << "    \"nx\": " << grid.gridSize().nx << ",\n";
    file << "    \"ny\": " << grid.gridSize().ny << ",\n";
    file << "    \"nz\": " << grid.gridSize().nz << ",\n";
    file << "    \"cell_count\": " << grid.cellCount() << "\n";
    file << "  },\n";
    file << "  \"material\": {\n";
    file << "    \"name\": \"" << jsonEscape(result.material.name) << "\",\n";
    file << "    \"heat_loss_coefficient\": " << result.material.heatLossCoefficient << ",\n";
    file << "    \"solar_transmission\": " << result.material.solarTransmission << "\n";
    file << "  },\n";
    file << "  \"total_heater_energy_kwh\": " << result.totalHeaterEnergyKWh;

    if (finalFrame != nullptr) {
        file << ",\n";
        file << "  \"final\": {\n";
        file << "    \"time_seconds\": " << finalFrame->timeSeconds << ",\n";
        file << "    \"average_temperature_c\": "
             << finalFrame->climate.temperatureStep.temperature.averageTemperatureC << ",\n";
        file << "    \"min_temperature_c\": "
             << finalFrame->climate.temperatureStep.temperature.minTemperatureC << ",\n";
        file << "    \"max_temperature_c\": "
             << finalFrame->climate.temperatureStep.temperature.maxTemperatureC << ",\n";
        file << "    \"average_humidity_percent\": "
             << finalFrame->climate.humidity.averageHumidityPercent << ",\n";
        file << "    \"plant_average_temperature_c\": "
             << finalFrame->plantTemperature.averageTemperatureC << ",\n";
        file << "    \"plant_average_temperature_error_c\": "
             << finalFrame->plantTemperature.averageAbsoluteErrorC << ",\n";
        file << "    \"plant_average_humidity_percent\": "
             << finalFrame->plantHumidity.averageHumidityPercent << "\n";
        file << "  }\n";
    } else {
        file << "\n";
    }

    file << "}\n";
}

} // namespace

ExportedFiles exportSimulationResult(
    const SimulationResult& result,
    const Grid3D& grid,
    const OutputSpec& output
) {
    ExportedFiles files;
    const std::filesystem::path directory = ensureOutputDirectory(output);

    if (output.writeCsv) {
        const std::filesystem::path cellPath = directory / "cell_history.csv";
        const std::filesystem::path metricsPath = directory / "metrics.csv";
        writeCellHistoryCsv(result, grid, cellPath);
        writeMetricsCsv(result, metricsPath);

        files.cellHistoryCsvPath = cellPath.string();
        files.metricsCsvPath = metricsPath.string();
        files.paths.push_back(files.cellHistoryCsvPath);
        files.paths.push_back(files.metricsCsvPath);
    }

    if (output.writeJson) {
        const std::filesystem::path summaryPath = directory / "summary.json";
        writeSummaryJson(result, grid, summaryPath);

        files.summaryJsonPath = summaryPath.string();
        files.paths.push_back(files.summaryJsonPath);
    }

    return files;
}

} // namespace greenhouse
