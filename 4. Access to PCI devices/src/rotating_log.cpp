#include "../include/rotating_log.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

RotatingLog::RotatingLog(const std::string& filename, RetentionPolicy policy)
    : filename_(filename), policy_(policy) {
    std::cout << "Log file: " << filename_ << std::endl;
}

RotatingLog::~RotatingLog() {
    // Просто выводим сообщение для отладки
    std::cout << "Closing log: " << filename_ << std::endl;
}

void RotatingLog::write(const std::string& entry) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ofstream file(filename_, std::ios::app);
    if (file.is_open()) {
        file << entry << std::endl;
        std::cout << "Logged: " << entry << std::endl;
    } else {
        std::cerr << "Failed to open log file: " << filename_ << std::endl;
    }

    remove_old_entries();
}

void RotatingLog::write(const TemperatureMeasurement& measurement) {
    write(measurement.to_string());
}

std::vector<std::string> RotatingLog::read_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> lines;
    std::ifstream file(filename_);

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
    }

    return lines;
}

std::vector<TemperatureMeasurement> RotatingLog::read_measurements() const {
    // Упрощенная реализация - возвращаем пустой вектор
    return {};
}

double RotatingLog::calculate_average(const std::chrono::system_clock::time_point& start,
                           const std::chrono::system_clock::time_point& end) const {
    // Упрощенная реализация - игнорируем параметры
    (void)start; (void)end;  // Подавляем предупреждения о неиспользуемых параметрах
    return 0.0;
}

void RotatingLog::remove_old_entries() {
    try {
        // Проверяем размер файла
        if (std::filesystem::exists(filename_)) {
            auto size = std::filesystem::file_size(filename_);
            if (size > 10 * 1024 * 1024) { // 10 MB лимит
                std::filesystem::remove(filename_);
                std::cout << "Rotated log file: " << filename_ << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error rotating log: " << e.what() << std::endl;
    }
}

std::chrono::system_clock::time_point RotatingLog::get_oldest_allowed() const {
    return std::chrono::system_clock::now() - std::chrono::hours(24);
}