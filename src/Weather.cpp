#include "Weather.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace greenhouse {

namespace {

double clampHumidity(double humidityPercent) {
    return std::clamp(humidityPercent, 0.0, 100.0);
}

WeatherCondition makeCondition(
    double timeSeconds,
    double outsideTemperatureC,
    double outsideHumidityPercent,
    double solarRadiationWm2
) {
    return {
        timeSeconds,
        outsideTemperatureC,
        clampHumidity(outsideHumidityPercent),
        std::max(0.0, solarRadiationWm2)
    };
}

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> result;
    std::stringstream stream(line);
    std::string token;
    while (std::getline(stream, token, ',')) {
        result.push_back(token);
    }
    return result;
}

bool tryParseWeatherLine(const std::string& line, WeatherCondition& condition) {
    const std::vector<std::string> values = splitCsvLine(line);
    if (values.size() != 4) {
        return false;
    }

    try {
        condition = makeCondition(
            std::stod(values[0]),
            std::stod(values[1]),
            std::stod(values[2]),
            std::stod(values[3])
        );
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

WeatherCondition interpolate(
    const WeatherCondition& before,
    const WeatherCondition& after,
    double timeSeconds
) {
    const double duration = after.timeSeconds - before.timeSeconds;
    if (duration <= 0.0) {
        return before;
    }

    const double t = (timeSeconds - before.timeSeconds) / duration;
    return makeCondition(
        timeSeconds,
        before.outsideTemperatureC
            + (after.outsideTemperatureC - before.outsideTemperatureC) * t,
        before.outsideHumidityPercent
            + (after.outsideHumidityPercent - before.outsideHumidityPercent) * t,
        before.solarRadiationWm2
            + (after.solarRadiationWm2 - before.solarRadiationWm2) * t
    );
}

} // namespace

WeatherTimeline::WeatherTimeline(const WeatherSpec& fallback)
    : fallback_(makeCondition(
          0.0,
          fallback.outsideTemperatureC,
          fallback.outsideHumidityPercent,
          fallback.solarRadiationWm2
      )) {
}

WeatherTimeline WeatherTimeline::fromCsv(
    const std::string& path,
    const WeatherSpec& fallback
) {
    if (path.empty()) {
        return WeatherTimeline(fallback);
    }

    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open weather file: " + path);
    }

    WeatherTimeline timeline(fallback);
    timeline.source_ = path;

    std::string line;
    bool firstLine = true;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        if (firstLine) {
            firstLine = false;
            if (line.find("time_seconds") != std::string::npos) {
                continue;
            }
        }

        WeatherCondition condition;
        if (tryParseWeatherLine(line, condition)) {
            timeline.samples_.push_back(condition);
        }
    }

    std::sort(
        timeline.samples_.begin(),
        timeline.samples_.end(),
        [](const WeatherCondition& left, const WeatherCondition& right) {
            return left.timeSeconds < right.timeSeconds;
        }
    );

    return timeline;
}

WeatherCondition WeatherTimeline::at(double timeSeconds) const {
    if (samples_.empty()) {
        WeatherCondition condition = fallback_;
        condition.timeSeconds = timeSeconds;
        return condition;
    }

    if (timeSeconds <= samples_.front().timeSeconds) {
        return samples_.front();
    }

    if (timeSeconds >= samples_.back().timeSeconds) {
        return samples_.back();
    }

    const auto after = std::lower_bound(
        samples_.begin(),
        samples_.end(),
        timeSeconds,
        [](const WeatherCondition& condition, double value) {
            return condition.timeSeconds < value;
        }
    );

    if (after == samples_.begin()) {
        return *after;
    }

    return interpolate(*(after - 1), *after, timeSeconds);
}

bool WeatherTimeline::usesCsv() const {
    return !samples_.empty();
}

std::size_t WeatherTimeline::sampleCount() const {
    return samples_.size();
}

const std::string& WeatherTimeline::source() const {
    return source_;
}

} // namespace greenhouse
