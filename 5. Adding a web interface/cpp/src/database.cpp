#include "../include/database.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>

Database::Database(const std::string& connection_string)
    : connection_string_(connection_string) {
}

Database::~Database() {
    if (connection_ && connection_->is_open()) {
        connection_->close();
    }
}

bool Database::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        connection_ = std::make_unique<pqxx::connection>(connection_string_);
        
        if (connection_->is_open()) {
            std::cout << "Connected to PostgreSQL database: " 
                      << connection_->dbname() << std::endl;
            return true;
        } else {
            std::cerr << "Failed to connect to PostgreSQL" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
        return false;
    }
}

bool Database::is_connected() const {
    return connection_ && connection_->is_open();
}

bool Database::add_measurement(double temperature) {
    if (!is_connected()) return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        pqxx::work txn(*connection_);
        
        std::stringstream ss;
        ss << "INSERT INTO measurements (temperature) VALUES (" 
           << txn.quote(temperature) << ")";
        
        txn.exec(ss.str());
        txn.commit();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error adding measurement: " << e.what() << std::endl;
        return false;
    }
}

bool Database::add_hourly_average(double average, const std::chrono::system_clock::time_point& timestamp) {
    if (!is_connected()) return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        pqxx::work txn(*connection_);
        
        std::string timestamp_str = time_point_to_string(timestamp);
        std::stringstream ss;
        ss << "INSERT INTO hourly_averages (timestamp, average_temperature) VALUES ("
           << txn.quote(timestamp_str) << ", "
           << txn.quote(average) << ")";
        
        txn.exec(ss.str());
        txn.commit();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error adding hourly average: " << e.what() << std::endl;
        return false;
    }
}

bool Database::add_daily_average(double average, const std::chrono::system_clock::time_point& timestamp) {
    if (!is_connected()) return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        pqxx::work txn(*connection_);
        
        std::string timestamp_str = time_point_to_string(timestamp);
        std::stringstream ss;
        ss << "INSERT INTO daily_averages (timestamp, average_temperature) VALUES ("
           << txn.quote(timestamp_str) << ", "
           << txn.quote(average) << ")";
        
        txn.exec(ss.str());
        txn.commit();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error adding daily average: " << e.what() << std::endl;
        return false;
    }
}

std::vector<TemperatureRecord> Database::get_measurements(int limit) {
    std::vector<TemperatureRecord> results;
    
    if (!is_connected()) return results;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        pqxx::work txn(*connection_);
        
        std::stringstream ss;
        ss << "SELECT timestamp, temperature FROM measurements "
           << "ORDER BY timestamp DESC LIMIT " << limit;
        
        pqxx::result res = txn.exec(ss.str());
        
        for (const auto& row : res) {
            std::string timestamp_str = row["timestamp"].as<std::string>();
            double temperature = row["temperature"].as<double>();
            
            results.emplace_back(string_to_time_point(timestamp_str), temperature);
        }
        
        return results;
    } catch (const std::exception& e) {
        std::cerr << "Error getting measurements: " << e.what() << std::endl;
        return results;
    }
}

std::vector<TemperatureRecord> Database::get_hourly_averages(int days) {
    std::vector<TemperatureRecord> results;
    
    if (!is_connected()) return results;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        pqxx::work txn(*connection_);
        
        std::stringstream ss;
        ss << "SELECT timestamp, average_temperature as temperature "
           << "FROM hourly_averages "
           << "WHERE timestamp >= NOW() - INTERVAL '" << days << " days' "
           << "ORDER BY timestamp";
        
        pqxx::result res = txn.exec(ss.str());
        
        for (const auto& row : res) {
            std::string timestamp_str = row["timestamp"].as<std::string>();
            double temperature = row["temperature"].as<double>();
            
            results.emplace_back(string_to_time_point(timestamp_str), temperature);
        }
        
        return results;
    } catch (const std::exception& e) {
        std::cerr << "Error getting hourly averages: " << e.what() << std::endl;
        return results;
    }
}

std::vector<TemperatureRecord> Database::get_daily_averages(int days) {
    std::vector<TemperatureRecord> results;
    
    if (!is_connected()) return results;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        pqxx::work txn(*connection_);
        
        std::stringstream ss;
        ss << "SELECT DATE(timestamp) as date, "
           << "AVG(average_temperature) as temperature "
           << "FROM daily_averages "
           << "WHERE timestamp >= NOW() - INTERVAL '" << days << " days' "
           << "GROUP BY DATE(timestamp) "
           << "ORDER BY date";
        
        pqxx::result res = txn.exec(ss.str());
        
        for (const auto& row : res) {
            std::string date_str = row["date"].as<std::string>();
            double temperature = row["temperature"].as<double>();
            
            // Преобразуем дату в полноценный timestamp
            std::tm tm = {};
            std::stringstream date_ss(date_str);
            date_ss >> std::get_time(&tm, "%Y-%m-%d");
            auto timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            
            results.emplace_back(timestamp, temperature);
        }
        
        return results;
    } catch (const std::exception& e) {
        std::cerr << "Error getting daily averages: " << e.what() << std::endl;
        return results;
    }
}

TemperatureRecord Database::get_current_temperature() {
    if (!is_connected()) return TemperatureRecord();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        pqxx::work txn(*connection_);
        
        std::string query = "SELECT timestamp, temperature "
                           "FROM measurements "
                           "ORDER BY timestamp DESC LIMIT 1";
        
        pqxx::result res = txn.exec(query);
        
        if (!res.empty()) {
            std::string timestamp_str = res[0]["timestamp"].as<std::string>();
            double temperature = res[0]["temperature"].as<double>();
            
            return TemperatureRecord(string_to_time_point(timestamp_str), temperature);
        }
        
        return TemperatureRecord();
    } catch (const std::exception& e) {
        std::cerr << "Error getting current temperature: " << e.what() << std::endl;
        return TemperatureRecord();
    }
}

TemperatureRecord Database::get_statistics(const std::string& period) {
    if (!is_connected()) return TemperatureRecord();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        pqxx::work txn(*connection_);
        std::string query;
        
        if (period == "hour") {
            query = "SELECT AVG(temperature) as avg_temp FROM measurements "
                   "WHERE timestamp >= NOW() - INTERVAL '1 hour'";
        } else if (period == "day") {
            query = "SELECT AVG(temperature) as avg_temp FROM measurements "
                   "WHERE timestamp >= NOW() - INTERVAL '1 day'";
        } else if (period == "week") {
            query = "SELECT AVG(average_temperature) as avg_temp FROM hourly_averages "
                   "WHERE timestamp >= NOW() - INTERVAL '7 days'";
        } else if (period == "month") {
            query = "SELECT AVG(average_temperature) as avg_temp FROM daily_averages "
                   "WHERE timestamp >= NOW() - INTERVAL '30 days'";
        } else {
            return TemperatureRecord();
        }
        
        pqxx::result res = txn.exec(query);
        
        if (!res.empty() && !res[0]["avg_temp"].is_null()) {
            double avg_temp = res[0]["avg_temp"].as<double>();
            return TemperatureRecord(std::chrono::system_clock::now(), avg_temp);
        }
        
        return TemperatureRecord();
    } catch (const std::exception& e) {
        std::cerr << "Error getting statistics: " << e.what() << std::endl;
        return TemperatureRecord();
    }
}

void Database::cleanup_old_data() {
    if (!is_connected()) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        pqxx::work txn(*connection_);
        
        txn.exec("SELECT cleanup_old_measurements()");
        txn.commit();
        
        std::cout << "Old data cleaned up" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error cleaning up old data: " << e.what() << std::endl;
    }
}

void Database::test_connection() {
    if (!is_connected()) {
        std::cerr << "Database is not connected" << std::endl;
        return;
    }
    
    try {
        pqxx::work txn(*connection_);
        pqxx::result res = txn.exec("SELECT 1 as test");
        
        if (!res.empty() && res[0]["test"].as<int>() == 1) {
            std::cout << "Database connection test: OK" << std::endl;
        } else {
            std::cerr << "Database connection test: FAILED" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Database connection test error: " << e.what() << std::endl;
    }
}

std::string Database::time_point_to_string(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::chrono::system_clock::time_point Database::string_to_time_point(const std::string& str) {
    std::tm tm = {};
    std::stringstream ss(str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

pqxx::result Database::execute_query(const std::string& query) {
    if (!is_connected()) throw std::runtime_error("Database not connected");
    
    pqxx::work txn(*connection_);
    return txn.exec(query);
}