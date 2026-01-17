#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <vector>
#include <ctime>

class Database;

struct LiveUpdate {
    time_t timestamp;
    double temperature;
};

class WebServer {
private:
    Database& db;
    int port;
    int serverSocket;
    bool running;

    // Буфер для последних измерений (для long-polling)
    std::vector<LiveUpdate> recentMeasurements;
    time_t lastClientPollTime;

    void handleClient(int clientSocket);
    std::string getContentType(const std::string& path);
    std::string readFile(const std::string& path);

    // API обработчики
    std::string handleApiCurrent();
    std::string handleApiRange(const std::string& query);
    std::string handleApiStats(const std::string& query);
    std::string handleApiLive(const std::string& query);  // Новый эндпоинт

    // Утилиты
    time_t parseIsoTime(const std::string& isoTime);
    std::string timeToIsoString(time_t t);

public:
    WebServer(Database& db, int port = 8080);
    ~WebServer();

    void start();
    void stop();
    bool isRunning() const { return running; }

    // Метод для добавления нового измерения (вызывается из основного потока)
    void addMeasurement(time_t timestamp, double temperature);
};

#endif // WEBSERVER_H