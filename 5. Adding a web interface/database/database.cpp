#include "database.h"
#include <iostream>

Database::Database(const std::string& db_name) {
    sqlite3_open(db_name.c_str(), &db);
    std::string clean_sql = "DROP TABLE IF EXISTS temp_logs;";
    sqlite3_exec(db, clean_sql.c_str(), nullptr, nullptr, nullptr);
    std::string sql = "CREATE TABLE IF NOT EXISTS temp_logs ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
                      "temperature REAL);";
    sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
}

Database::~Database() {
    sqlite3_close(db);
}

void Database::insert_temp(double temp) {
    std::string sql = "INSERT INTO temp_logs (temperature) VALUES (" + std::to_string(temp) + ");";
    sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
}

std::string Database::get_current_json() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT temperature, timestamp FROM temp_logs ORDER BY id DESC LIMIT 1", -1, &stmt, nullptr);
    std::string json = "{}";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        json = "{\"temp\":" + std::to_string(sqlite3_column_double(stmt, 0)) + 
               ", \"time\":\"" + (const char*)sqlite3_column_text(stmt, 1) + "\"}";
    }
    sqlite3_finalize(stmt);
    return json;
}

std::string Database::get_history_json(int seconds) {
    sqlite3_stmt* stmt;
    std::string sql = "SELECT timestamp, temperature FROM temp_logs "
                      "WHERE timestamp >= datetime('now', '-" + std::to_string(seconds) + " seconds') "
                      "ORDER BY id DESC";
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    std::string json = "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) json += ",";
        json += "{\"time\":\"" + std::string((const char*)sqlite3_column_text(stmt, 0)) + "\",";
        json += "\"temp\":" + std::to_string(sqlite3_column_double(stmt, 1)) + "}";
        first = false;
    }
    json += "]";
    sqlite3_finalize(stmt);
    return json;
}