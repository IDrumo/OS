#include "../include/config.h"
#include "../include/temperature_measurement.h"
#include "../include/rotating_log.h"
#include "../include/data_processor.h"
#include "../include/temperature_validator.h"
#include "../include/serial.h"

#include <iostream>
#include <memory>
#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>
#include <fstream>
#include <iomanip>

class TemperatureMonitor {
public:
    TemperatureMonitor(const Config& config)
        : config_(config)
        , measurements_log_("measurements.log", RotatingLog::RetentionPolicy::LAST_24_HOURS)
        , hourly_log_("hourly.log", RotatingLog::RetentionPolicy::LAST_30_DAYS)
        , daily_log_("daily.log", RotatingLog::RetentionPolicy::CURRENT_YEAR)
        , processor_(config.buffer_size)
        , validator_(config.min_temperature, config.max_temperature)
        , running_(false) {
    }

    bool initialize() {
        try {
            serial_port_ = std::make_unique<SerialPort>();

            if (!serial_port_->open(config_.port,
                                   static_cast<SerialPort::BaudRate>(config_.baud_rate))) {
                std::cerr << "Failed to open serial port: " << config_.port << std::endl;
                return false;
            }

            serial_port_->set_timeout(1.0);

            // Инициализация лог-файлов
            std::ofstream test1("measurements.log", std::ios::app);
            std::ofstream test2("hourly.log", std::ios::app);
            std::ofstream test3("daily.log", std::ios::app);

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Serial port initialization failed: " << e.what() << std::endl;
            return false;
        }
    }

    void run() {
        running_ = true;
        setup_signal_handlers();

        auto last_hour_check = std::chrono::steady_clock::now();
        auto last_day_check = std::chrono::steady_clock::now();

        std::cout << "Temperature monitor started. Reading data..." << std::endl;

        while (running_) {
            // Чтение из порта
            std::string line = serial_port_->read_line();

            if (!line.empty()) {
                process_line(line);
            }

            // Проверка смены часа
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_hour_check).count() >= 5) { // 5 секунд для теста
                flush_hourly_average();
                last_hour_check = now;
            }

            // Проверка смены дня
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_day_check).count() >= 10) { // 10 секунд для теста
                flush_daily_average();
                last_day_check = now;
            }

            // Небольшая задержка
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Сохранение оставшихся данных при завершении
        flush_all();
    }

    void stop() {
        running_ = false;
    }

private:
    void process_line(const std::string& line) {
        auto result = validator_.validate(line);

        if (result.valid) {
            TemperatureMeasurement measurement(result.temperature);

            std::cout << "✓ New measurement: " << std::fixed << std::setprecision(2)
                      << result.temperature << "°C" << std::endl;

            processor_.add_measurement(measurement);
            processor_.add_to_hourly(result.temperature);
            processor_.add_to_daily(result.temperature);

            // Запись в лог измерений
            measurements_log_.write(measurement);

            // Проверка на заполнение буфера
            if (processor_.should_flush()) {
                std::cout << "  Buffer flushed to disk" << std::endl;
            }
        } else {
            std::cerr << "✗ Invalid data: " << result.error_message
                      << " (raw: " << line << ")" << std::endl;
        }
    }

    void flush_hourly_average() {
        double avg = processor_.get_hourly_average();
        size_t count = processor_.get_hourly_count();

        if (count > 0) {
            std::cout << "\n--- Hourly average: " << std::fixed << std::setprecision(2)
                      << avg << "°C (based on " << count << " measurements) ---" << std::endl;

            TemperatureMeasurement measurement(avg);
            hourly_log_.write(measurement);
            processor_.reset_hourly();
        }
    }

    void flush_daily_average() {
        double avg = processor_.get_daily_average();
        size_t count = processor_.get_daily_count();

        if (count > 0) {
            std::cout << "\n=== Daily average: " << std::fixed << std::setprecision(2)
                      << avg << "°C (based on " << count << " measurements) ===" << std::endl;

            TemperatureMeasurement measurement(avg);
            daily_log_.write(measurement);
            processor_.reset_daily();
        }
    }

    void flush_all() {
        auto measurements = processor_.flush();
        for (const auto& m : measurements) {
            measurements_log_.write(m);
        }

        if (processor_.get_hourly_count() > 0) {
            flush_hourly_average();
        }

        if (processor_.get_daily_count() > 0) {
            flush_daily_average();
        }
    }

    static void signal_handler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            std::cout << "\nShutting down..." << std::endl;
            instance_->stop();
        }
    }

    void setup_signal_handlers() {
        instance_ = this;
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
    }

    Config config_;
    RotatingLog measurements_log_;
    RotatingLog hourly_log_;
    RotatingLog daily_log_;
    DataProcessor processor_;
    TemperatureValidator validator_;
    std::unique_ptr<SerialPort> serial_port_;
    std::atomic<bool> running_;

    static TemperatureMonitor* instance_;
};

TemperatureMonitor* TemperatureMonitor::instance_ = nullptr;

int main(int argc, char** argv) {
    try {
        Config config = Config::from_command_line(argc, argv);

        TemperatureMonitor monitor(config);

        if (!monitor.initialize()) {
            return 1;
        }

        std::cout << "========================================" << std::endl;
        std::cout << "Temperature Monitor v1.0" << std::endl;
        std::cout << "Port: " << config.port << std::endl;
        std::cout << "Baud rate: " << config.baud_rate << std::endl;
        std::cout << "Log files: measurements.log, hourly.log, daily.log" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        std::cout << "========================================" << std::endl;

        monitor.run();

        std::cout << "Temperature monitor stopped" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}