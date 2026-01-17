#include "../include/data_processor.h"
#include <numeric>  // для std::accumulate
#include <iostream>

DataProcessor::DataProcessor(size_t buffer_size)
    : buffer_size_(buffer_size) {
    buffer_.reserve(buffer_size);
}

void DataProcessor::add_measurement(const TemperatureMeasurement& measurement) {
    buffer_.push_back(measurement);
}

bool DataProcessor::should_flush() const {
    return buffer_.size() >= buffer_size_;
}

std::vector<TemperatureMeasurement> DataProcessor::flush() {
    std::vector<TemperatureMeasurement> result;
    result.swap(buffer_);
    buffer_.clear();
    buffer_.reserve(buffer_size_);
    return result;
}

void DataProcessor::reset_hourly() {
    hourly_measurements_.clear();
}

void DataProcessor::reset_daily() {
    daily_measurements_.clear();
}

void DataProcessor::add_to_hourly(double temperature) {
    hourly_measurements_.push_back(temperature);
}

void DataProcessor::add_to_daily(double temperature) {
    daily_measurements_.push_back(temperature);
}

double DataProcessor::get_hourly_average() const {
    if (hourly_measurements_.empty()) return 0.0;

    double sum = std::accumulate(hourly_measurements_.begin(),
                                 hourly_measurements_.end(), 0.0);
    return sum / hourly_measurements_.size();
}

double DataProcessor::get_daily_average() const {
    if (daily_measurements_.empty()) return 0.0;

    double sum = std::accumulate(daily_measurements_.begin(),
                                 daily_measurements_.end(), 0.0);
    return sum / daily_measurements_.size();
}

size_t DataProcessor::get_hourly_count() const {
    return hourly_measurements_.size();
}

size_t DataProcessor::get_daily_count() const {
    return daily_measurements_.size();
}