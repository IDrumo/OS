#ifndef TEMPERATURE_VALIDATOR_H
#define TEMPERATURE_VALIDATOR_H

#include <string>
#include <utility>  // для std::pair
#include <optional>

class TemperatureValidator {
public:
    struct ValidationResult {
        bool valid;
        double temperature;
        std::string error_message;
    };

    TemperatureValidator(double min_temp = -60.0, double max_temp = 60.0);

    ValidationResult validate(const std::string& raw_data) const;
    bool validate_checksum(const std::string& data, const std::string& checksum) const;
    bool is_temperature_in_range(double temperature) const;

private:
    double min_temperature_;
    double max_temperature_;

    std::string calculate_checksum(const std::string& data) const;
    std::optional<std::pair<double, std::string>> parse_packet(const std::string& packet) const;
};

#endif