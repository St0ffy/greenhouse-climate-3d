#pragma once

#include "Types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace greenhouse {

struct WeatherCondition {
    double timeSeconds = 0.0;
    double outsideTemperatureC = 5.0;
    double outsideHumidityPercent = 80.0;
    double solarRadiationWm2 = 300.0;
};

class WeatherTimeline {
public:
    explicit WeatherTimeline(const WeatherSpec& fallback);

    static WeatherTimeline fromCsv(
        const std::string& path,
        const WeatherSpec& fallback
    );

    WeatherCondition at(double timeSeconds) const;

    bool usesCsv() const;
    std::size_t sampleCount() const;
    const std::string& source() const;

private:
    WeatherCondition fallback_;
    std::vector<WeatherCondition> samples_;
    std::string source_ = "constant config";
};

} // namespace greenhouse
