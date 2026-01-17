#include "../include/http_server.h"
#include "../include/database.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <json.hpp>

using json = nlohmann::json;

HttpServer::HttpServer(Database* db, int port)
    : db_(db), port_(port), server_(nullptr), running_(false) {
}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    if (running_) return true;

    server_ = std::make_unique<httplib::Server>();
    setup_routes();

    server_thread_ = std::thread([this]() {
        std::cout << "Starting HTTP server on port " << port_ << std::endl;
        running_ = true;

        if (!server_->bind_to_port("0.0.0.0", port_)) {
            std::cerr << "Failed to bind to port " << port_ << std::endl;
            running_ = false;
            return;
        }

        if (!server_->listen_after_bind()) {
            std::cerr << "Failed to start listening on port " << port_ << std::endl;
        }

        running_ = false;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return running_;
}

void HttpServer::stop() {
    if (server_) {
        server_->stop();
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    running_ = false;
}

void HttpServer::setup_routes() {
    // Текущая температура
    server_->Get("/api/current", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto current = db_->get_current_temperature();

            json j;
            if (current.temperature != 0.0) {
                j["temperature"] = current.temperature;
                j["timestamp"] = format_timestamp(current.timestamp);
                j["success"] = true;
            } else {
                j["success"] = false;
                j["error"] = "No data available";
            }

            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            json j;
            j["success"] = false;
            j["error"] = e.what();
            res.status = 500;
            res.set_content(j.dump(), "application/json");
        }
    });

    // Здоровье системы
    server_->Get("/api/health", [this](const httplib::Request& req, httplib::Response& res) {
        json j;
        j["status"] = "ok";
        j["timestamp"] = format_timestamp(std::chrono::system_clock::now());
        j["database_connected"] = db_->is_connected();
        res.set_content(j.dump(), "application/json");
    });

    // Получение измерений
    server_->Get("/api/measurements", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            int limit = 100;
            if (req.has_param("limit")) {
                limit = std::stoi(req.get_param_value("limit"));
                if (limit > 10000) limit = 10000; // Безопасный лимит
            }

            auto measurements = db_->get_measurements(limit);
            json j;
            j["success"] = true;
            j["count"] = measurements.size();
            j["data"] = json::array();

            for (const auto& record : measurements) {
                json item;
                item["timestamp"] = format_timestamp(record.timestamp);
                item["temperature"] = record.temperature;
                j["data"].push_back(item);
            }

            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            json j;
            j["success"] = false;
            j["error"] = e.what();
            res.status = 500;
            res.set_content(j.dump(), "application/json");
        }
    });

    // Статистика за период
    server_->Get("/api/statistics", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string period = "hour";
            if (req.has_param("period")) {
                period = req.get_param_value("period");
            }

            auto stats = db_->get_statistics(period);
            json j;

            if (stats.temperature != 0.0) {
                j["success"] = true;
                j["period"] = period;
                j["average"] = stats.temperature;
                j["timestamp"] = format_timestamp(stats.timestamp);
            } else {
                j["success"] = false;
                j["error"] = "No data available for period: " + period;
            }

            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            json j;
            j["success"] = false;
            j["error"] = e.what();
            res.status = 500;
            res.set_content(j.dump(), "application/json");
        }
    });

    // Часовые средние
    server_->Get("/api/hourly", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            int days = 7;
            if (req.has_param("days")) {
                days = std::stoi(req.get_param_value("days"));
                if (days > 365) days = 365;
            }

            auto hourly = db_->get_hourly_averages(days);
            json j;
            j["success"] = true;
            j["count"] = hourly.size();
            j["data"] = json::array();

            for (const auto& record : hourly) {
                json item;
                item["timestamp"] = format_timestamp(record.timestamp);
                item["temperature"] = record.temperature;
                j["data"].push_back(item);
            }

            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            json j;
            j["success"] = false;
            j["error"] = e.what();
            res.status = 500;
            res.set_content(j.dump(), "application/json");
        }
    });

    // Дневные средние
    server_->Get("/api/daily", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            int days = 30;
            if (req.has_param("days")) {
                days = std::stoi(req.get_param_value("days"));
                if (days > 365) days = 365;
            }

            auto daily = db_->get_daily_averages(days);
            json j;
            j["success"] = true;
            j["count"] = daily.size();
            j["data"] = json::array();

            for (const auto& record : daily) {
                json item;
                item["timestamp"] = format_timestamp(record.timestamp);
                item["temperature"] = record.temperature;
                j["data"].push_back(item);
            }

            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            json j;
            j["success"] = false;
            j["error"] = e.what();
            res.status = 500;
            res.set_content(j.dump(), "application/json");
        }
    });

    // Очистка старых данных
    server_->Post("/api/cleanup", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            db_->cleanup_old_data();

            json j;
            j["success"] = true;
            j["message"] = "Old data cleaned up successfully";

            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            json j;
            j["success"] = false;
            j["error"] = e.what();
            res.status = 500;
            res.set_content(j.dump(), "application/json");
        }
    });

    // Статические файлы для веб-интерфейса
    server_->set_mount_point("/", "./web");
}

std::string HttpServer::format_timestamp(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}