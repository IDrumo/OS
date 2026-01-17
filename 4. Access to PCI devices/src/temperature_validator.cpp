#include "../include/temperature_validator.h"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>

TemperatureValidator::TemperatureValidator(double min_temp, double max_temp)
    : min_temperature_(min_temp), max_temperature_(max_temp) {}

TemperatureValidator::ValidationResult TemperatureValidator::validate(const std::string& raw_data) const {
    ValidationResult result{false, 0.0, "Invalid format"};

    auto parsed = parse_packet(raw_data);
    if (!parsed) {
        result.error_message = "Failed to parse packet";
        return result;
    }

    auto [temperature, checksum] = parsed.value();

    if (!is_temperature_in_range(temperature)) {
        result.error_message = "Temperature out of range";
        return result;
    }

    if (!validate_checksum(std::to_string(temperature), checksum)) {
        result.error_message = "Checksum mismatch";
        return result;
    }

    result.valid = true;
    result.temperature = temperature;
    result.error_message = "";

    return result;
}

bool TemperatureValidator::validate_checksum(const std::string& data, const std::string& checksum) const {
    // Реальная проверка контрольной суммы
    std::string calculated = calculate_checksum(data);

    // Приводим к верхнему регистру для сравнения
    std::string upper_checksum = checksum;
    std::string upper_calculated = calculated;

    std::transform(upper_checksum.begin(), upper_checksum.end(), upper_checksum.begin(), ::toupper);
    std::transform(upper_calculated.begin(), upper_calculated.end(), upper_calculated.begin(), ::toupper);

    return upper_calculated == upper_checksum;
}

bool TemperatureValidator::is_temperature_in_range(double temperature) const {
    return temperature >= min_temperature_ && temperature <= max_temperature_;
}

std::string TemperatureValidator::calculate_checksum(const std::string& data) const {
    // Простой XOR checksum
    unsigned char checksum = 0;
    for (char c : data) {
        checksum ^= static_cast<unsigned char>(c);
    }

    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(checksum);
    return ss.str();
}

std::optional<std::pair<double, std::string>> TemperatureValidator::parse_packet(const std::string& packet) const {
    std::stringstream ss(packet);
    double temperature;
    std::string checksum;

    if (ss >> temperature >> checksum) {
        return std::make_pair(temperature, checksum);
    }

    return std::nullopt;
}