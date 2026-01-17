#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <chrono>

struct Config {
    // File paths
    std::string measurements_log = "measurements.log";
    std::string hourly_log = "hourly.log";
    std::string daily_log = "daily.log";

    // Retention periods (в часах вместо дней)
    std::chrono::hours measurements_retention{24};
    std::chrono::hours hourly_retention{30 * 24};  // 30 дней
    std::chrono::hours daily_retention{365 * 24};  // 365 дней

    // Serial port settings
    std::string port;
    int baud_rate = 115200;

    // Data validation
    double min_temperature = -60.0;
    double max_temperature = 60.0;
    size_t buffer_size = 10;

    static Config from_command_line(int argc, char** argv);
};

#endif