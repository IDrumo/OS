#pragma once

#include <chrono>
#include <string>

struct TemperatureMeasurement {
    std::chrono::system_clock::time_point timestamp;
    double temperature;

    TemperatureMeasurement(double temp);
    TemperatureMeasurement(std::chrono::system_clock::time_point ts, double temp);

    std::string to_string() const;
    static TemperatureMeasurement from_string(const std::string& str);
};