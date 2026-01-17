#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <thread>
#include <memory>
#include <httplib.h>
#include <vector>

#include "database.h"

class Database;

class HttpServer {
public:
    HttpServer(Database* db, int port = 8080);
    ~HttpServer();

    bool start();
    void stop();

private:
    Database* db_;
    int port_;
    std::unique_ptr<httplib::Server> server_;
    std::thread server_thread_;
    bool running_;

    void setup_routes();
    std::string to_json(const std::vector<TemperatureRecord>& records);
    std::string format_timestamp(const std::chrono::system_clock::time_point& tp);
};

#endif // HTTP_SERVER_H