#include "database.h"
#include <sqlite3.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

Database::Database() : db(nullptr) {}

Database::~Database() {
    close();
}

bool Database::open(const std::string& path) {
    if (db) close();

    int rc = sqlite3_open(path.c_str(), (sqlite3**)&db);
    if (rc != SQLITE_OK) {
        std::cerr << "Не удалось открыть базу данных: "
                  << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return false;
    }

    // Создаем таблицу измерений
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS measurements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            temperature REAL NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_timestamp ON measurements(timestamp);
    )";

    char* errMsg = nullptr;
    rc = sqlite3_exec((sqlite3*)db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка создания таблицы: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        close();
        return false;
    }

    return true;
}

void Database::close() {
    if (db) {
        sqlite3_close((sqlite3*)db);
        db = nullptr;
    }
}

bool Database::insertMeasurement(time_t timestamp, double temperature) {
    if (!db) return false;

    const char* sql = "INSERT INTO measurements (timestamp, temperature) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка подготовки SQL: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)timestamp);
    sqlite3_bind_double(stmt, 2, temperature);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<Measurement> Database::getMeasurements(time_t from, time_t to) {
    std::vector<Measurement> results;
    if (!db) return results;

    const char* sql = "SELECT timestamp, temperature FROM measurements "
                      "WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка подготовки SQL: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return results;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)from);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)to);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Measurement m{};
        m.timestamp = (time_t)sqlite3_column_int64(stmt, 0);
        m.temperature = sqlite3_column_double(stmt, 1);
        results.push_back(m);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<Measurement> Database::getLatestMeasurements(int limit) {
    std::vector<Measurement> results;
    if (!db) return results;

    const char* sql = "SELECT timestamp, temperature FROM measurements "
                      "ORDER BY timestamp DESC LIMIT ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка подготовки SQL: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return results;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Measurement m{};
        m.timestamp = (time_t)sqlite3_column_int64(stmt, 0);
        m.temperature = sqlite3_column_double(stmt, 1);
        results.push_back(m);
    }

    sqlite3_finalize(stmt);
    return results;
}

Statistics Database::getStatistics(time_t from, time_t to) {
    Statistics stats{};
    stats.count = 0;
    stats.average = 0.0;
    stats.min = 0.0;
    stats.max = 0.0;

    if (!db) return stats;

    const char* sql = "SELECT COUNT(*), AVG(temperature), MIN(temperature), MAX(temperature) "
                      "FROM measurements WHERE timestamp >= ? AND timestamp <= ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка подготовки SQL: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return stats;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)from);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)to);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stats.count = sqlite3_column_int(stmt, 0);
        stats.average = sqlite3_column_double(stmt, 1);
        stats.min = sqlite3_column_double(stmt, 2);
        stats.max = sqlite3_column_double(stmt, 3);
    }

    sqlite3_finalize(stmt);
    return stats;
}

bool Database::cleanupOldData(int daysToKeep) {
    if (!db) return false;

    // Получаем текущее время
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    // Вычисляем время отсечения (текущее время - daysToKeep дней)
    time_t cutoff = now_time_t - (daysToKeep * 24 * 3600);

    const char* sql = "DELETE FROM measurements WHERE timestamp < ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка подготовки SQL: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)cutoff);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        // Освобождаем место в базе данных
        sqlite3_exec((sqlite3*)db, "VACUUM;", nullptr, nullptr, nullptr);
        return true;
    }

    return false;
}