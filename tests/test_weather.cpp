#include "Weather.h"

#include <cassert>
#include <cmath>

namespace {

bool almostEqual(double left, double right) {
    return std::fabs(left - right) < 1e-9;
}

} // namespace

int main() {
    const greenhouse::WeatherSpec fallback{5.0, 75.0, 350.0, ""};
    const greenhouse::WeatherTimeline constantWeather(fallback);
    const greenhouse::WeatherCondition constant = constantWeather.at(1000.0);
    assert(almostEqual(constant.outsideTemperatureC, 5.0));
    assert(almostEqual(constant.outsideHumidityPercent, 75.0));
    assert(almostEqual(constant.solarRadiationWm2, 350.0));

    const greenhouse::WeatherTimeline timeline =
        greenhouse::WeatherTimeline::fromCsv("data/weather_sample.csv", fallback);
    assert(timeline.usesCsv());
    assert(timeline.sampleCount() == 7);

    const greenhouse::WeatherCondition middle = timeline.at(1800.0);
    assert(almostEqual(middle.outsideTemperatureC, 4.5));
    assert(almostEqual(middle.outsideHumidityPercent, 81.0));
    assert(almostEqual(middle.solarRadiationWm2, 60.0));

    const greenhouse::WeatherCondition end = timeline.at(999999.0);
    assert(almostEqual(end.outsideTemperatureC, 6.0));

    return 0;
}
