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
constexpr double kTemperatureSatisfiedToleranceC = 0.5;
constexpr double kHumiditySatisfiedTolerancePercent = 2.0;

struct ValueRange {
    double minValue = 0.0;
    double maxValue = 1.0;
};

enum class TerminalKey {
    None,
    Escape,
    Up,
    Down,
    Left,
    Right,
    Enter,
    ToggleFailure,
    CycleField,
    LayerDown,
    LayerUp,
    Pause
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

    TerminalKey pollKey() {
        if (!interactive_) {
            return TerminalKey::None;
        }

#ifdef _WIN32
        if (_kbhit() == 0) {
            return TerminalKey::None;
        }

        const int ch = _getch();
        if (ch == 0 || ch == 224) {
            if (_kbhit() == 0) {
                return TerminalKey::None;
            }
            switch (_getch()) {
                case 72:
                    return TerminalKey::Up;
                case 80:
                    return TerminalKey::Down;
                case 75:
                    return TerminalKey::Left;
                case 77:
                    return TerminalKey::Right;
                default:
                    return TerminalKey::None;
            }
        }

        return keyFromChar(static_cast<char>(ch));
#else
        if (!active_) {
            return TerminalKey::None;
        }

        fd_set readSet;
        timeval timeout{};
        FD_ZERO(&readSet);
        FD_SET(STDIN_FILENO, &readSet);

        if (select(STDIN_FILENO + 1, &readSet, nullptr, nullptr, &timeout) <= 0) {
            return TerminalKey::None;
        }

        char ch = '\0';
        if (read(STDIN_FILENO, &ch, 1) != 1) {
            return TerminalKey::None;
        }

        if (ch == 27) {
            char seq[2] = {'\0', '\0'};
            if (read(STDIN_FILENO, &seq[0], 1) == 1
                && read(STDIN_FILENO, &seq[1], 1) == 1
                && seq[0] == '[') {
                switch (seq[1]) {
                    case 'A':
                        return TerminalKey::Up;
                    case 'B':
                        return TerminalKey::Down;
                    case 'C':
                        return TerminalKey::Right;
                    case 'D':
                        return TerminalKey::Left;
                    default:
                        return TerminalKey::None;
                }
            }
            return TerminalKey::Escape;
        }

        return keyFromChar(ch);
#endif
    }

    bool escapePressed() {
        return pollKey() == TerminalKey::Escape;
    }

private:
    static TerminalKey keyFromChar(char ch) {
        switch (ch) {
            case 27:
                return TerminalKey::Escape;
            case '\n':
            case '\r':
                return TerminalKey::Enter;
            case 'w':
            case 'W':
                return TerminalKey::Up;
            case 's':
            case 'S':
                return TerminalKey::Down;
            case 'a':
            case 'A':
                return TerminalKey::Left;
            case 'd':
            case 'D':
                return TerminalKey::Right;
            case 'f':
            case 'F':
                return TerminalKey::ToggleFailure;
            case 't':
            case 'T':
                return TerminalKey::CycleField;
            case 'z':
            case 'Z':
                return TerminalKey::LayerDown;
            case 'x':
            case 'X':
                return TerminalKey::LayerUp;
            case ' ':
                return TerminalKey::Pause;
            default:
                return TerminalKey::None;
        }
    }

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

    if (field == "light" || field == "light_w_m2") {
        return "light";
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

    if (activeField == "light") {
        return cell.lightWm2;
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

bool samePlanCell(const GridIndex& left, const GridIndex& right) {
    return left.x == right.x && left.y == right.y;
}

bool sensorNeedsClimateWork(
    const MappedPlantPoint& plant,
    const SimulationFrame& frame
) {
    for (const PlantSensorReading& sensor : frame.sensorReadings) {
        if (sensor.plantName == plant.plant.name) {
            return sensor.temperatureC
                    < plant.plant.targetTemperatureC - kTemperatureSatisfiedToleranceC
                || sensor.humidityPercent
                    < plant.plant.targetHumidityPercent - kHumiditySatisfiedTolerancePercent;
        }
    }

    return false;
}

bool sensorTooHigh(
    const MappedPlantPoint& plant,
    const SimulationFrame& frame
) {
    for (const PlantSensorReading& sensor : frame.sensorReadings) {
        if (sensor.plantName == plant.plant.name) {
            return sensor.temperatureC
                    > plant.plant.targetTemperatureC + kTemperatureSatisfiedToleranceC
                || sensor.humidityPercent
                    > plant.plant.targetHumidityPercent + kHumiditySatisfiedTolerancePercent;
        }
    }

    return false;
}

char sensorSymbolForPlant(
    const MappedPlantPoint& plant,
    const SimulationFrame& frame
) {
    if (sensorNeedsClimateWork(plant, frame)) {
        return 'S';
    }

    if (sensorTooHigh(plant, frame)) {
        return '!';
    }

    return 's';
}

std::string sensorStatusText(
    const MappedPlantPoint& plant,
    const SimulationFrame& frame
) {
    if (sensorNeedsClimateWork(plant, frame)) {
        return "needs heat/humidity";
    }
    if (sensorTooHigh(plant, frame)) {
        return "needs venting";
    }
    return "stable";
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
    const SimulationFrame& frame,
    bool projectDevicesToLayer
) {
    int deviceCount = 0;
    char symbol = '\0';

    for (const MappedPlantPoint& plant : devices.plants) {
        if (sameViewCell(plant.sensorCell, x, y, z, projectDevicesToLayer)) {
            addDeviceSymbol(sensorSymbolForPlant(plant, frame), deviceCount, symbol);
            continue;
        }

        if (sameViewCell(plant.cell, x, y, z, projectDevicesToLayer)) {
            addDeviceSymbol('P', deviceCount, symbol);
        }
    }

    for (const MappedHeater& heater : devices.heaters) {
        if (sameViewCell(heater.anchorCell, x, y, z, projectDevicesToLayer)) {
            addDeviceSymbol(heater.spec.failed ? 'h' : 'H', deviceCount, symbol);
        }
    }

    for (const MappedVent& vent : devices.vents) {
        if (sameViewCell(vent.anchorCell, x, y, z, projectDevicesToLayer)) {
            addDeviceSymbol(vent.spec.failed ? 'v' : 'V', deviceCount, symbol);
        }
    }

    for (const MappedHumidifier& humidifier : devices.humidifiers) {
        if (sameViewCell(humidifier.anchorCell, x, y, z, projectDevicesToLayer)) {
            addDeviceSymbol(humidifier.spec.failed ? 'm' : 'M', deviceCount, symbol);
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
    const SimulationFrame& frame,
    bool projectDevicesToLayer
) {
    int deviceCount = 0;
    char symbol = '\0';

    for (const MappedPlantPoint& plant : devices.plants) {
        if (cellInViewBlock(plant.sensorCell, minX, maxX, minY, maxY, z, projectDevicesToLayer)) {
            addDeviceSymbol(sensorSymbolForPlant(plant, frame), deviceCount, symbol);
            continue;
        }

        if (cellInViewBlock(plant.cell, minX, maxX, minY, maxY, z, projectDevicesToLayer)) {
            addDeviceSymbol('P', deviceCount, symbol);
        }
    }

    for (const MappedHeater& heater : devices.heaters) {
        if (cellInViewBlock(heater.anchorCell, minX, maxX, minY, maxY, z, projectDevicesToLayer)) {
            addDeviceSymbol(heater.spec.failed ? 'h' : 'H', deviceCount, symbol);
        }
    }

    for (const MappedVent& vent : devices.vents) {
        if (cellInViewBlock(vent.anchorCell, minX, maxX, minY, maxY, z, projectDevicesToLayer)) {
            addDeviceSymbol(vent.spec.failed ? 'v' : 'V', deviceCount, symbol);
        }
    }

    for (const MappedHumidifier& humidifier : devices.humidifiers) {
        if (cellInViewBlock(humidifier.anchorCell, minX, maxX, minY, maxY, z, projectDevicesToLayer)) {
            addDeviceSymbol(humidifier.spec.failed ? 'm' : 'M', deviceCount, symbol);
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
        case 'S':
            return "\033[33m";
        case 's':
            return "\033[92m";
        case '!':
            return "\033[91m";
        case 'H':
        case 'h':
            return "\033[31m";
        case 'V':
        case 'v':
            return "\033[36m";
        case 'M':
        case 'm':
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

void printSymbol(char symbol, const char* color, bool useColors, bool highlighted = false) {
    if (useColors) {
        if (highlighted) {
            std::cout << "\033[7m";
        }
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
              << " % | plant uptake avg: "
              << frame.climate.averagePlantHumidityUptakePercent
              << " %\n";
    std::cout << "plant health avg: "
              << frame.plantGrowth.averageHealth
              << " | growth avg: "
              << frame.plantGrowth.averageGrowth
              << " | alive: "
              << frame.plantGrowth.aliveCount
              << "\n";
    std::cout << "controller reward: "
              << frame.control.reward
              << " | heater power: "
              << frame.control.activeHeaterPowerW
              << " W | failed devices: "
              << frame.control.failedDeviceCount
              << "\n";
    std::cout << "heater energy: "
              << frame.cumulativeHeaterEnergyKWh
              << " kWh | device energy: "
              << frame.cumulativeDeviceEnergyKWh
              << " kWh\n";
}

void printSelectedCellInfo(
    const SimulationFrame& frame,
    const Grid3D& grid,
    const MappedDeviceSet& devices,
    const GridIndex& cursor,
    const std::string& message
) {
    if (!grid.contains(cursor)) {
        return;
    }

    const std::size_t linear = grid.linearIndex(cursor);
    if (linear >= frame.cells.size()) {
        return;
    }

    const CellState& cell = frame.cells[linear];
    std::cout << "\nSelected cell: " << grid.formatIndex(cursor) << "\n";
    std::cout << "cell temp: " << cell.temperatureC
              << " C | humidity: " << cell.humidityPercent
              << " % | light: " << cell.lightWm2 << " W/m2\n";

    for (const MappedPlantPoint& plant : devices.plants) {
        if (samePlanCell(plant.cell, cursor) || samePlanCell(plant.sensorCell, cursor)) {
            std::cout << "plant: " << plant.plant.name
                      << " | target temp " << plant.plant.targetTemperatureC
                      << " C | target humidity "
                      << plant.plant.targetHumidityPercent << " %\n";
            for (const PlantSensorReading& sensor : frame.sensorReadings) {
                if (sensor.plantName == plant.plant.name) {
                    std::cout << "sensor: " << grid.formatIndex(sensor.sensorCell)
                              << " | status " << sensorStatusText(plant, frame)
                              << " | temp " << sensor.temperatureC
                              << " C | humidity " << sensor.humidityPercent
                              << " % | light " << sensor.lightWm2 << " W/m2\n";
                }
            }
        }
    }

    for (const MappedHeater& heater : devices.heaters) {
        if (samePlanCell(heater.anchorCell, cursor)) {
            std::cout << "heater: " << heater.spec.name
                      << " | power " << heater.spec.powerW
                      << "/" << effectiveHeaterMaxPowerW(heater.spec)
                      << " W | failed "
                      << (heater.spec.failed ? "yes" : "no") << "\n";
        }
    }

    for (const MappedHumidifier& humidifier : devices.humidifiers) {
        if (samePlanCell(humidifier.anchorCell, cursor)) {
            std::cout << "humidifier: " << humidifier.spec.name
                      << " | mode " << humidifier.spec.mode
                      << " | level " << humidifier.spec.level
                      << " | failed "
                      << (humidifier.spec.failed ? "yes" : "no") << "\n";
        }
    }

    for (const MappedVent& vent : devices.vents) {
        if (samePlanCell(vent.anchorCell, cursor)) {
            std::cout << "vent: " << vent.spec.name
                      << " | opening " << vent.spec.opening
                      << " | failed "
                      << (vent.spec.failed ? "yes" : "no") << "\n";
        }
    }

    if (!message.empty()) {
        std::cout << "message: " << message << "\n";
    }
}

void printFrame(
    const SimulationFrame& frame,
    const Grid3D& grid,
    const MappedDeviceSet& devices,
    const TerminalViewSpec& view,
    const GridIndex* cursor = nullptr,
    const std::string& message = {}
) {
    const MappedDeviceSet& activeDevices =
        frame.devices.plants.empty() ? devices : frame.devices;
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
    if (view.interactive) {
        std::cout << "Interactive: arrows/WASD move | Enter/F fail/repair device | T field | Z/X layer | Space pause | Esc finish\n\n";
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
                    activeDevices,
                    frame,
                    view.projectDevicesToLayer
                );

                if (device != '\0') {
                    symbol = device;
                    color = deviceColor(device);
                }
            }

            const bool highlighted =
                cursor != nullptr
                && cursor->x >= minX
                && cursor->x <= maxX
                && cursor->y >= minY
                && cursor->y <= maxY;
            printSymbol(symbol, color, view.useColors, highlighted);
        }

        std::cout << "\n";
    }

    std::cout << "\nLegend:\n";
    std::cout << "low  . : - = + * # % @  high\n";
    std::cout << "P plant | S sensor active | s sensor stable | ! sensor high | H heater | V vent | M humidifier | lowercase failed | * multiple devices\n";

    printStats(frame);
    if (cursor != nullptr) {
        printSelectedCellInfo(frame, grid, activeDevices, *cursor, message);
    }
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

void moveCursor(GridIndex& cursor, const Grid3D& grid, TerminalKey key) {
    GridIndex next = cursor;
    switch (key) {
        case TerminalKey::Up:
            ++next.y;
            break;
        case TerminalKey::Down:
            --next.y;
            break;
        case TerminalKey::Left:
            --next.x;
            break;
        case TerminalKey::Right:
            ++next.x;
            break;
        default:
            break;
    }

    const GridSize size = grid.gridSize();
    next.x = std::clamp(next.x, 0, size.nx - 1);
    next.y = std::clamp(next.y, 0, size.ny - 1);
    next.z = std::clamp(next.z, 0, size.nz - 1);
    cursor = next;
}

void cycleField(TerminalViewSpec& view) {
    const std::string field = normalizedField(view.field);
    if (field == "temperature") {
        view.field = "humidity";
    } else if (field == "humidity") {
        view.field = "light";
    } else if (field == "light") {
        view.field = "temperature_error";
    } else {
        view.field = "temperature";
    }
}

bool handleInteractiveKey(
    TerminalKey key,
    SimulationStepper& stepper,
    const Grid3D& grid,
    TerminalViewSpec& view,
    GridIndex& cursor,
    bool& paused,
    std::string& message
) {
    switch (key) {
        case TerminalKey::None:
            return true;
        case TerminalKey::Escape:
            return false;
        case TerminalKey::Up:
        case TerminalKey::Down:
        case TerminalKey::Left:
        case TerminalKey::Right:
            moveCursor(cursor, grid, key);
            return true;
        case TerminalKey::Enter:
        case TerminalKey::ToggleFailure:
            if (stepper.toggleDeviceFailureAt(cursor)) {
                message = "device failure state changed";
            } else {
                message = "no device at selected cell";
            }
            return true;
        case TerminalKey::CycleField:
            cycleField(view);
            return true;
        case TerminalKey::LayerDown:
            view.layerZ = std::max(0, view.layerZ - 1);
            cursor.z = clampedLayerZ(grid, view.layerZ);
            return true;
        case TerminalKey::LayerUp:
            view.layerZ = std::min(grid.gridSize().nz - 1, view.layerZ + 1);
            cursor.z = clampedLayerZ(grid, view.layerZ);
            return true;
        case TerminalKey::Pause:
            paused = !paused;
            message = paused ? "paused" : "running";
            return true;
    }

    return true;
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

SimulationResult runInteractiveSimulationInTerminal(
    const SimulationConfig& config,
    const Grid3D& grid,
    const WeatherTimeline& weather,
    const MaterialProperties& material,
    const MappedDeviceSet& devices,
    const ClimatePhysicsSettings& settings
) {
    TerminalViewSpec view = config.output.terminalView;
    if (!view.enabled || !view.interactive) {
        return runSimulation(config, grid, weather, material, devices, settings);
    }

    TerminalInput input;
    if (!input.isInteractive()) {
        return runSimulation(config, grid, weather, material, devices, settings);
    }

    SimulationConfig activeConfig = config;
    activeConfig.output.terminalView = view;
    SimulationStepper stepper(
        activeConfig,
        grid,
        weather,
        material,
        devices,
        settings
    );

    GridIndex cursor{
        0,
        0,
        clampedLayerZ(grid, view.layerZ)
    };
    bool paused = false;
    bool running = true;
    std::string message;

    while (running) {
        view.layerZ = clampedLayerZ(grid, view.layerZ);
        cursor.z = view.layerZ;
        printFrame(
            stepper.currentFrame(),
            grid,
            stepper.devices(),
            view,
            &cursor,
            message
        );

        if (stepper.finished()) {
            break;
        }

        message.clear();
        const int sleepMs = std::max(25, view.sleepMs);
        int elapsed = 0;
        while (elapsed < sleepMs) {
            const TerminalKey key = input.pollKey();
            if (key != TerminalKey::None) {
                running = handleInteractiveKey(
                    key,
                    stepper,
                    grid,
                    view,
                    cursor,
                    paused,
                    message
                );
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            elapsed += 25;
        }

        if (!running) {
            break;
        }

        if (!paused) {
            stepper.step();
        }
    }

    return stepper.result();
}

} // namespace greenhouse
