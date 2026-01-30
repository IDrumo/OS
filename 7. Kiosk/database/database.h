#pragma once
#include <string>
#include <vector>
#include "sqlite3.h"

class Database {
public:
    Database(const std::string& db_name);
    ~Database();
    void insert_temp(double temp);
    std::string get_current_json();
    std::string get_history_json(int seconds);
private:
    sqlite3* db;
};