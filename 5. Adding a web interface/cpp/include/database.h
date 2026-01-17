#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <memory>
#include <pqxx/pqxx>

struct TemperatureRecord {
    std::chrono::system_clock::time_point timestamp;
    double temperature;

    TemperatureRecord() = default;
    TemperatureRecord(std::chrono::system_clock::time_point ts, double temp)
        : timestamp(ts), temperature(temp) {}
};

class Database {
public:
    Database(const std::string& connection_string);
    ~Database();

    bool connect();
    bool is_connected() const;

    bool add_measurement(double temperature);
    bool add_hourly_average(double average, const std::chrono::system_clock::time_point& timestamp);
    bool add_daily_average(double average, const std::chrono::system_clock::time_point& timestamp);

    std::vector<TemperatureRecord> get_measurements(int limit = 1000);
    std::vector<TemperatureRecord> get_measurements_range(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end);

    std::vector<TemperatureRecord> get_hourly_averages(int days = 30);
    std::vector<TemperatureRecord> get_daily_averages(int days = 365);

    TemperatureRecord get_current_temperature();
    TemperatureRecord get_statistics(const std::string& period);

    void cleanup_old_data();
    void test_connection();

private:
    std::string connection_string_;
    std::unique_ptr<pqxx::connection> connection_;
    mutable std::mutex mutex_;

    std::string time_point_to_string(const std::chrono::system_clock::time_point& tp);
    std::chrono::system_clock::time_point string_to_time_point(const std::string& str);
    pqxx::result execute_query(const std::string& query);
};

#endif // DATABASE_H