#include "../include/temperature_measurement.h"
#include <sstream>
#include <iomanip>
#include <ctime>

TemperatureMeasurement::TemperatureMeasurement(double temp)
    : timestamp(std::chrono::system_clock::now()), temperature(temp) {}

TemperatureMeasurement::TemperatureMeasurement(std::chrono::system_clock::time_point ts, double temp)
    : timestamp(ts), temperature(temp) {}

std::string TemperatureMeasurement::to_string() const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::tm* tm = std::localtime(&time_t);

    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S")
       << " " << std::fixed << std::setprecision(2) << temperature;
    return ss.str();
}

TemperatureMeasurement TemperatureMeasurement::from_string(const std::string& str) {
    (void)str;  // Подавляем предупреждение
    // Упрощенная реализация - возвращаем текущее время
    return TemperatureMeasurement(std::chrono::system_clock::now(), 0.0);
}