#include "../include/config.h"
#include "../include/temperature_measurement.h"
#include "../include/data_processor.h"
#include "../include/temperature_validator.h"
#include "../include/serial.h"
#include "../include/database.h"
#include "../include/http_server.h"

#include <iostream>
#include <memory>
#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>
#include <iomanip>

class TemperatureMonitor {
public:
    TemperatureMonitor(const Config& config)
        : config_(config)
        , processor_(config.buffer_size)
        , validator_(config.min_temperature, config.max_temperature)
        , db_(std::make_unique<Database>())
        , http_server_(nullptr)
        , running_(false) {
    }

    bool initialize() {
        try {
            // Инициализация базы данных
            if (!db_->initialize()) {
                std::cerr << "Failed to initialize database" << std::endl;
                return false;
            }

            // Запуск HTTP сервера
            http_server_ = std::make_unique<HttpServer>(db_.get(), 8080);
            if (!http_server_->start()) {
                std::cerr << "Failed to start HTTP server" << std::endl;
                return false;
            }

            // Открытие COM-порта
            serial_port_ = std::make_unique<SerialPort>();

            if (!serial_port_->open(config_.port,
                                   static_cast<SerialPort::BaudRate>(config_.baud_rate))) {
                std::cerr << "Failed to open serial port: " << config_.port << std::endl;
                return false;
            }

            serial_port_->set_timeout(1.0);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Initialization failed: " << e.what() << std::endl;
            return false;
        }
    }

    void run() {
        running_ = true;
        setup_signal_handlers();

        auto last_hour_check = std::chrono::steady_clock::now();
        auto last_day_check = std::chrono::steady_clock::now();
        auto last_cleanup = std::chrono::steady_clock::now();

        std::cout << "Temperature monitor started. Reading data..." << std::endl;
        std::cout << "HTTP server: http://localhost:8080" << std::endl;

        while (running_) {
            // Чтение из порта
            std::string line = serial_port_->read_line();

            if (!line.empty()) {
                process_line(line);
            }

            auto now = std::chrono::steady_clock::now();

            // Проверка смены часа
            if (now - last_hour_check >= std::chrono::hours(1)) {
                flush_hourly_average();
                last_hour_check = now;
            }

            // Проверка смены дня
            if (now - last_day_check >= std::chrono::hours(24)) {
                flush_daily_average();
                last_day_check = now;
            }

            // Очистка старых данных
            if (now - last_cleanup >= std::chrono::hours(6)) {
                db_->cleanup_old_data();
                last_cleanup = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

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
                      << result.temperature << " C" << std::endl;

            // Сохранение в базу данных
            db_->add_measurement(result.temperature);

            processor_.add_measurement(measurement);
            processor_.add_to_hourly(result.temperature);
            processor_.add_to_daily(result.temperature);

            if (processor_.should_flush()) {
                std::cout << "  Buffer flushed" << std::endl;
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
                      << avg << " C (based on " << count << " measurements) ---" << std::endl;

            db_->add_hourly_average(avg, std::chrono::system_clock::now());
            processor_.reset_hourly();
        }
    }

    void flush_daily_average() {
        double avg = processor_.get_daily_average();
        size_t count = processor_.get_daily_count();

        if (count > 0) {
            std::cout << "\n=== Daily average: " << std::fixed << std::setprecision(2)
                      << avg << " C (based on " << count << " measurements) ===" << std::endl;

            db_->add_daily_average(avg, std::chrono::system_clock::now());
            processor_.reset_daily();
        }
    }

    void flush_all() {
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
    DataProcessor processor_;
    TemperatureValidator validator_;
    std::unique_ptr<SerialPort> serial_port_;
    std::unique_ptr<Database> db_;
    std::unique_ptr<HttpServer> http_server_;
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
        std::cout << "Temperature Monitor v2.0" << std::endl;
        std::cout << "Port: " << config.port << std::endl;
        std::cout << "Database: temperature.db" << std::endl;
        std::cout << "Web interface: http://localhost:8080" << std::endl;
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