// data_processor.hpp
#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include "temperature_measurement.h"

class DataProcessor {
public:
    DataProcessor(size_t buffer_size = 10);

    void add_measurement(const TemperatureMeasurement& measurement);
    bool should_flush() const;
    std::vector<TemperatureMeasurement> flush();

    // Statistics
    void reset_hourly();
    void reset_daily();
    void add_to_hourly(double temperature);
    void add_to_daily(double temperature);

    double get_hourly_average() const;
    double get_daily_average() const;
    size_t get_hourly_count() const;
    size_t get_daily_count() const;

private:
    std::vector<TemperatureMeasurement> buffer_;
    size_t buffer_size_;

    std::vector<double> hourly_measurements_;
    std::vector<double> daily_measurements_;
};