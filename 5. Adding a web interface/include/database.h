#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <ctime>

struct Measurement {
    time_t timestamp;
    double temperature;
};

struct Statistics {
    int count;
    double average;
    double min;
    double max;
};

class Database {
private:
    void* db; // sqlite3* - скрываем реализацию

public:
    Database();
    ~Database();

    bool open(const std::string& path);
    void close();

    bool insertMeasurement(time_t timestamp, double temperature);
    std::vector<Measurement> getMeasurements(time_t from, time_t to);
    std::vector<Measurement> getLatestMeasurements(int limit);
    Statistics getStatistics(time_t from, time_t to);
    bool cleanupOldData(int daysToKeep); // Добавляем этот метод!

    // Запрещаем копирование
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
};

#endif // DATABASE_H