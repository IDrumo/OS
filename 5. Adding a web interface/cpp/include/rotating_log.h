#ifndef ROTATING_LOG_H
#define ROTATING_LOG_H

#include <string>
#include <chrono>
#include <vector>
#include <mutex>
#include "temperature_measurement.h"

class RotatingLog {
public:
    enum class RetentionPolicy {
        LAST_24_HOURS,
        LAST_30_DAYS,
        CURRENT_YEAR
    };

    RotatingLog(const std::string& filename, RetentionPolicy policy);
    ~RotatingLog();  // Убрали = default, будем определять в .cpp

    void write(const std::string& entry);
    void write(const TemperatureMeasurement& measurement);

    std::vector<std::string> read_all() const;
    std::vector<TemperatureMeasurement> read_measurements() const;

    double calculate_average(const std::chrono::system_clock::time_point& start,
                           const std::chrono::system_clock::time_point& end) const;

private:
    void rotate();
    void remove_old_entries();
    std::chrono::system_clock::time_point get_oldest_allowed() const;

    std::string filename_;
    RetentionPolicy policy_;
    mutable std::mutex mutex_;
};

#endif // ROTATING_LOG_H