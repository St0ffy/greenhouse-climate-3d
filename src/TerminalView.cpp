#include "TerminalView.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <cstdio>
#include <io.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace greenhouse {

namespace {

constexpr const char* kScale = " .:-=+*#%@";

struct ValueRange {
    double minValue = 0.0;
    double maxValue = 1.0;
};

class TerminalInput {
public:
    TerminalInput() {
#ifdef _WIN32
        interactive_ = _isatty(_fileno(stdin)) != 0;
#else
        interactive_ = isatty(STDIN_FILENO) != 0;
        if (interactive_ && tcgetattr(STDIN_FILENO, &original_) == 0) {
            termios raw = original_;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            active_ = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
        }
#endif
    }

    TerminalInput(const TerminalInput&) = delete;
    TerminalInput& operator=(const TerminalInput&) = delete;

    ~TerminalInput() {
#ifndef _WIN32
        if (active_) {
            tcsetattr(STDIN_FILENO, TCSANOW, &original_);
        }
#endif
    }

    bool isInteractive() const {
        return interactive_;
    }

    bool escapePressed() {
        if (!interactive_) {
            return false;
        }

#ifdef _WIN32
        while (_kbhit() != 0) {
            if (_getch() == 27) {
                return true;
            }
        }
        return false;
#else
        if (!active_) {
            return false;
        }

        fd_set readSet;
        timeval timeout{};
        FD_ZERO(&readSet);
        FD_SET(STDIN_FILENO, &readSet);

        if (select(STDIN_FILENO + 1, &readSet, nullptr, nullptr, &timeout) <= 0) {
            return false;
        }

        char ch = '\0';
        while (read(STDIN_FILENO, &ch, 1) == 1) {
            if (ch == 27) {
                return true;
            }
        }

        return false;
#endif
    }

private:
    bool interactive_ = false;
#ifndef _WIN32
    bool active_ = false;
    termios original_{};
#endif
};

std::string normalizedField(const std::string& field) {
    if (field == "humidity" || field == "humidity_percent") {
        return "humidity";
    }

    if (field == "temperature_error" || field == "error") {
        return "temperature_error";
    }

    return "temperature";
}

double targetTemperatureForFrame(const SimulationFrame& frame) {
    if (frame.plantTemperature.averageTargetTemperatureC > 0.0) {
        return frame.plantTemperature.averageTargetTemperatureC;
    }

    return frame.climate.temperatureStep.temperature.averageTemperatureC;
}

double cellFieldValue(
    const CellState& cell,
    const SimulationFrame& frame,
    const std::string& field
) {
    const std::string activeField = normalizedField(field);

    if (activeField == "humidity") {
        return cell.humidityPercent;
    }

    if (activeField == "temperature_error") {
        return std::fabs(cell.temperatureC - targetTemperatureForFrame(frame));
    }

    return cell.temperatureC;
}

ValueRange adjustedRange(double minValue, double maxValue) {
    if (!std::isfinite(minValue) || !std::isfinite(maxValue)) {
        return {0.0, 1.0};
    }

    if (maxValue < minValue) {
        std::swap(minValue, maxValue);
    }

    if (std::fabs(maxValue - minValue) < 1e-9) {
        return {minValue - 1.0, maxValue + 1.0};
    }

    return {minValue, maxValue};
}

ValueRange frameValueRange(
    const SimulationFrame& frame,
    const TerminalViewSpec& view
) {
    if (!view.autoScale) {
        return adjustedRange(view.fixedMinValue, view.fixedMaxValue);
    }

    if (frame.cells.empty()) {
        return {0.0, 1.0};
    }

    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();

    for (const CellState& cell : frame.cells) {
        const double value = cellFieldValue(cell, frame, view.field);
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }

    return adjustedRange(minValue, maxValue);
}

double normalizedRatio(double value, const ValueRange& range) {
    const double ratio =
        (value - range.minValue) / (range.maxValue - range.minValue);
    return std::clamp(ratio, 0.0, 1.0);
}

char valueToSymbol(double value, const ValueRange& range) {
    const std::string scale = kScale;
    const double ratio = normalizedRatio(value, range);
    const std::size_t index = std::min(
        scale.size() - 1,
        static_cast<std::size_t>(ratio * static_cast<double>(scale.size() - 1))
    );

    return scale[index];
}

int positiveStride(int stride) {
    return std::max(1, stride);
}

bool sameViewCell(
    const GridIndex& deviceCell,
    int x,
    int y,
    int z,
    bool projectDevicesToLayer
) {
    if (deviceCell.x != x || deviceCell.y != y) {
        return false;
    }

    return projectDevicesToLayer || deviceCell.z == z;
}

bool cellInViewBlock(
    const GridIndex& deviceCell,
    int minX,
    int maxX,
    int minY,
    int maxY,
    int z,
    bool projectDevicesToLayer
) {
    if (deviceCell.x < minX || deviceCell.x > maxX) {
        return false;
    }

    if (deviceCell.y < minY || deviceCell.y > maxY) {
        return false;
    }

    return projectDevicesToLayer || deviceCell.z == z;
}

void addDeviceSymbol(
    char candidate,
    int& deviceCount,
    char& symbol
) {
    ++deviceCount;
    symbol = deviceCount == 1 ? candidate : '*';
}

char deviceSymbolAt(
    int x,
    int y,
    int z,
    const MappedDeviceSet& devices,
    bool projectDevicesToLayer
) {
    int deviceCount = 0;
    char symbol = '\0';

    for (const MappedPlantPoint& plant : devices.plants) {
        if (sameViewCell(plant.cell, x, y, z, projectDevicesToLayer)) {
            addDeviceSymbol('P', deviceCount, symbol);
        }
    }

    for (const MappedHeater& heater : devices.heaters) {
        if (sameViewCell(heater.anchorCell, x, y, z, projectDevicesToLayer)) {
            addDeviceSymbol('H', deviceCount, symbol);
        }
    }

    for (const MappedVent& vent : devices.vents) {
        if (sameViewCell(vent.anchorCell, x, y, z, projectDevicesToLayer)) {
            addDeviceSymbol('V', deviceCount, symbol);
        }
    }

    for (const MappedHumidifier& humidifier : devices.humidifiers) {
        if (sameViewCell(humidifier.anchorCell, x, y, z, projectDevicesToLayer)) {
            addDeviceSymbol('M', deviceCount, symbol);
        }
    }

    return symbol;
}

char deviceSymbolInBlock(
    int minX,
    int maxX,
    int minY,
    int maxY,
    int z,
    const MappedDeviceSet& devices,
    bool projectDevicesToLayer
) {
    int deviceCount = 0;
    char symbol = '\0';

    for (const MappedPlantPoint& plant : devices.plants) {
        if (cellInViewBlock(plant.cell, minX, maxX, minY, maxY, z, projectDevicesToLayer)) {
            addDeviceSymbol('P', deviceCount, symbol);
        }
    }

    for (const MappedHeater& heater : devices.heaters) {
        if (cellInViewBlock(heater.anchorCell, minX, maxX, minY, maxY, z, projectDevicesToLayer)) {
            addDeviceSymbol('H', deviceCount, symbol);
        }
    }

    for (const MappedVent& vent : devices.vents) {
        if (cellInViewBlock(vent.anchorCell, minX, maxX, minY, maxY, z, projectDevicesToLayer)) {
            addDeviceSymbol('V', deviceCount, symbol);
        }
    }

    for (const MappedHumidifier& humidifier : devices.humidifiers) {
        if (cellInViewBlock(humidifier.anchorCell, minX, maxX, minY, maxY, z, projectDevicesToLayer)) {
            addDeviceSymbol('M', deviceCount, symbol);
        }
    }

    return symbol;
}

const char* valueColor(double value, const ValueRange& range) {
    const double ratio = normalizedRatio(value, range);

    if (ratio < 0.25) {
        return "\033[36m";
    }
    if (ratio < 0.45) {
        return "\033[34m";
    }
    if (ratio < 0.65) {
        return "\033[37m";
    }
    if (ratio < 0.85) {
        return "\033[33m";
    }

    return "\033[31m";
}

const char* deviceColor(char symbol) {
    switch (symbol) {
        case 'P':
            return "\033[32m";
        case 'H':
            return "\033[31m";
        case 'V':
            return "\033[36m";
        case 'M':
            return "\033[35m";
        case '*':
            return "\033[97m";
        default:
            return "\033[0m";
    }
}

std::string formatTime(double timeSeconds) {
    const long long totalSeconds =
        static_cast<long long>(std::llround(std::max(0.0, timeSeconds)));
    const long long hours = totalSeconds / 3600;
    const long long minutes = (totalSeconds / 60) % 60;
    const long long seconds = totalSeconds % 60;

    std::ostringstream output;
    output << std::setfill('0')
           << std::setw(2) << hours << ':'
           << std::setw(2) << minutes << ':'
           << std::setw(2) << seconds;
    return output.str();
}

int clampedLayerZ(const Grid3D& grid, int requestedLayerZ) {
    return std::clamp(requestedLayerZ, 0, grid.gridSize().nz - 1);
}

double blockFieldValue(
    const SimulationFrame& frame,
    const Grid3D& grid,
    const TerminalViewSpec& view,
    int minX,
    int maxX,
    int minY,
    int maxY,
    int z
) {
    double total = 0.0;
    int count = 0;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const GridIndex index{x, y, z};
            const std::size_t linear = grid.linearIndex(index);
            if (linear < frame.cells.size()) {
                total += cellFieldValue(frame.cells[linear], frame, view.field);
                ++count;
            }
        }
    }

    if (count == 0) {
        return 0.0;
    }

    return total / static_cast<double>(count);
}

void printSymbol(char symbol, const char* color, bool useColors) {
    if (useColors) {
        std::cout << color << symbol << "\033[0m";
    } else {
        std::cout << symbol;
    }
}

void printStats(const SimulationFrame& frame) {
    std::cout << "\nStats:\n";
    std::cout << "avg temp: "
              << frame.climate.temperatureStep.temperature.averageTemperatureC
              << " C | min: "
              << frame.climate.temperatureStep.temperature.minTemperatureC
              << " C | max: "
              << frame.climate.temperatureStep.temperature.maxTemperatureC
              << " C\n";
    std::cout << "plant avg temp: "
              << frame.plantTemperature.averageTemperatureC
              << " C | target: "
              << frame.plantTemperature.averageTargetTemperatureC
              << " C | error: "
              << frame.plantTemperature.averageAbsoluteErrorC
              << " C\n";
    std::cout << "humidity avg: "
              << frame.climate.humidity.averageHumidityPercent
              << " %\n";
    std::cout << "energy: "
              << frame.cumulativeHeaterEnergyKWh
              << " kWh\n";
}

void printFrame(
    const SimulationFrame& frame,
    const Grid3D& grid,
    const MappedDeviceSet& devices,
    const TerminalViewSpec& view
) {
    const int layerZ = clampedLayerZ(grid, view.layerZ);
    const ValueRange range = frameValueRange(frame, view);
    const GridSize size = grid.gridSize();
    const int strideX = positiveStride(view.displayStrideX);
    const int strideY = positiveStride(view.displayStrideY);

    if (view.clearScreen) {
        std::cout << "\033[H\033[J";
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Greenhouse terminal view\n";
    std::cout << "Field: " << normalizedField(view.field) << "\n";
    std::cout << "Time: " << formatTime(frame.timeSeconds) << "\n";
    std::cout << "Layer z = " << layerZ << "\n";
    std::cout << "Range: " << range.minValue << " .. "
              << range.maxValue << "\n\n";
    if (strideX > 1 || strideY > 1) {
        std::cout << "Display stride: "
                  << strideX << " x " << strideY
                  << " grid cells per terminal symbol\n\n";
    }
    if (view.loopPlayback) {
        std::cout << "Loop playback: press Esc to stop animation\n\n";
    }

    for (int yTop = size.ny - 1; yTop >= 0; yTop -= strideY) {
        const int minY = std::max(0, yTop - strideY + 1);
        const int maxY = yTop;

        for (int minX = 0; minX < size.nx; minX += strideX) {
            const int maxX = std::min(size.nx - 1, minX + strideX - 1);
            const double value =
                blockFieldValue(frame, grid, view, minX, maxX, minY, maxY, layerZ);
            char symbol = valueToSymbol(value, range);
            const char* color = valueColor(value, range);

            if (view.showDevices) {
                const char device = deviceSymbolInBlock(
                    minX,
                    maxX,
                    minY,
                    maxY,
                    layerZ,
                    devices,
                    view.projectDevicesToLayer
                );

                if (device != '\0') {
                    symbol = device;
                    color = deviceColor(device);
                }
            }

            printSymbol(symbol, color, view.useColors);
        }

        std::cout << "\n";
    }

    std::cout << "\nLegend:\n";
    std::cout << "low  . : - = + * # % @  high\n";
    std::cout << "P plant | H heater | V vent | M humidifier | * multiple devices\n";

    printStats(frame);
    std::cout << std::flush;
}

bool sleepOrStop(int sleepMs, TerminalInput& input) {
    if (input.escapePressed()) {
        return true;
    }

    int remainingMs = sleepMs;
    while (remainingMs > 0) {
        const int chunkMs = std::min(remainingMs, 25);
        std::this_thread::sleep_for(std::chrono::milliseconds(chunkMs));
        remainingMs -= chunkMs;

        if (input.escapePressed()) {
            return true;
        }
    }

    return false;
}

bool replayOnce(
    const SimulationResult& result,
    const Grid3D& grid,
    const MappedDeviceSet& devices,
    const TerminalViewSpec& view,
    TerminalInput& input
) {
    double nextFrameTime = 0.0;
    const SimulationFrame* lastPrintedFrame = nullptr;

    for (const SimulationFrame& frame : result.frames) {
        if (input.escapePressed()) {
            return true;
        }

        if (view.frameIntervalSeconds > 0.0
            && frame.timeSeconds + 1e-9 < nextFrameTime) {
            continue;
        }

        printFrame(frame, grid, devices, view);
        lastPrintedFrame = &frame;

        if (view.frameIntervalSeconds > 0.0) {
            while (nextFrameTime <= frame.timeSeconds + 1e-9) {
                nextFrameTime += view.frameIntervalSeconds;
            }
        }

        if (view.sleepMs > 0 && sleepOrStop(view.sleepMs, input)) {
            return true;
        }
    }

    const SimulationFrame& finalFrame = result.frames.back();
    if (lastPrintedFrame != &finalFrame) {
        printFrame(finalFrame, grid, devices, view);
        if (view.sleepMs > 0 && sleepOrStop(view.sleepMs, input)) {
            return true;
        }
    }

    return input.escapePressed();
}

} // namespace

void replaySimulationInTerminal(
    const SimulationResult& result,
    const Grid3D& grid,
    const MappedDeviceSet& devices
) {
    const TerminalViewSpec& view = result.config.output.terminalView;

    if (!view.enabled || result.frames.empty()) {
        return;
    }

    TerminalInput input;

    if (!view.loopPlayback || !input.isInteractive()) {
        replayOnce(result, grid, devices, view, input);
        return;
    }

    while (true) {
        if (replayOnce(result, grid, devices, view, input)) {
            break;
        }
    }
}

} // namespace greenhouse
