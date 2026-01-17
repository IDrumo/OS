#include "webserver.h"
#include "database.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <thread>
#include <chrono>
#include <algorithm>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
    #define SHUT_RDWR SD_BOTH
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <signal.h>
#endif

using namespace std::chrono;

WebServer::WebServer(Database& db, int port)
    : db(db), port(port), serverSocket(-1), running(false), lastClientPollTime(0) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

WebServer::~WebServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void WebServer::addMeasurement(time_t timestamp, double temperature) {
    LiveUpdate update;
    update.timestamp = timestamp;
    update.temperature = temperature;

    // Добавляем в буфер
    recentMeasurements.push_back(update);

    // Ограничиваем размер буфера (храним последние 100 измерений)
    if (recentMeasurements.size() > 100) {
        recentMeasurements.erase(recentMeasurements.begin());
    }
}

time_t WebServer::parseIsoTime(const std::string& isoTime) {
    if (isoTime.empty()) return 0;

    struct tm tm_struct = {};

    // Формат: YYYY-MM-DDTHH:MM:SSZ
    if (sscanf(isoTime.c_str(), "%d-%d-%dT%d:%d:%dZ",
               &tm_struct.tm_year, &tm_struct.tm_mon, &tm_struct.tm_mday,
               &tm_struct.tm_hour, &tm_struct.tm_min, &tm_struct.tm_sec) == 6) {
        tm_struct.tm_year -= 1900;
        tm_struct.tm_mon -= 1;
        tm_struct.tm_isdst = -1;
    } else {
        std::cout << "Failed to parse time: " << isoTime << std::endl;
        return 0;
    }

#ifdef _WIN32
    return _mkgmtime(&tm_struct);
#else
    return timegm(&tm_struct);
#endif
}

std::string WebServer::timeToIsoString(time_t t) {
    struct tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buffer);
}

std::string WebServer::getContentType(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html; charset=utf-8";
    if (path.find(".css") != std::string::npos) return "text/css; charset=utf-8";
    if (path.find(".js") != std::string::npos) return "application/javascript; charset=utf-8";
    if (path.find(".json") != std::string::npos) return "application/json; charset=utf-8";
    return "text/plain; charset=utf-8";
}

std::string WebServer::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string WebServer::handleApiCurrent() {
    auto latest = db.getLatestMeasurements(1);

    std::ostringstream json;
    json << "{\"data\":[";

    for (size_t i = 0; i < latest.size(); i++) {
        if (i > 0) json << ",";
        json << "{\"timestamp\":\"" << timeToIsoString(latest[i].timestamp) << "\",";
        json << "\"temperature\":" << latest[i].temperature << "}";
    }

    json << "]}";

    return json.str();
}

std::string WebServer::handleApiRange(const std::string& query) {
    // Парсим параметры from и to
    size_t fromPos = query.find("from=");
    size_t toPos = query.find("&to=");

    if (fromPos == std::string::npos || toPos == std::string::npos) {
        return "{\"error\":\"Missing parameters\"}";
    }

    std::string fromStr = query.substr(fromPos + 5, toPos - fromPos - 5);
    std::string toStr = query.substr(toPos + 4);

    time_t from = parseIsoTime(fromStr);
    time_t to = parseIsoTime(toStr);

    if (from == 0 || to == 0) {
        return "{\"error\":\"Invalid time format\"}";
    }

    auto measurements = db.getMeasurements(from, to);

    std::ostringstream json;
    json << "{\"data\":[";

    for (size_t i = 0; i < measurements.size(); i++) {
        if (i > 0) json << ",";
        json << "{\"timestamp\":\"" << timeToIsoString(measurements[i].timestamp) << "\",";
        json << "\"temperature\":" << measurements[i].temperature << "}";
    }

    json << "]}";

    return json.str();
}

std::string WebServer::handleApiStats(const std::string& query) {
    size_t fromPos = query.find("from=");
    size_t toPos = query.find("&to=");

    if (fromPos == std::string::npos || toPos == std::string::npos) {
        return "{\"error\":\"Missing parameters\"}";
    }

    std::string fromStr = query.substr(fromPos + 5, toPos - fromPos - 5);
    std::string toStr = query.substr(toPos + 4);

    time_t from = parseIsoTime(fromStr);
    time_t to = parseIsoTime(toStr);

    if (from == 0 || to == 0) {
        return "{\"error\":\"Invalid time format\"}";
    }

    auto stats = db.getStatistics(from, to);

    std::ostringstream json;
    json << "{";
    json << "\"count\":" << stats.count << ",";
    json << "\"average\":" << stats.average << ",";
    json << "\"min\":" << stats.min << ",";
    json << "\"max\":" << stats.max;
    json << "}";

    return json.str();
}

std::string WebServer::handleApiLive(const std::string& query) {
    // Парсим параметр since (время последнего обновления)
    size_t sincePos = query.find("since=");
    time_t since = 0;

    if (sincePos != std::string::npos) {
        std::string sinceStr = query.substr(sincePos + 6);
        since = parseIsoTime(sinceStr);
    }

    std::vector<LiveUpdate> newMeasurements;

    // Находим измерения, которые произошли после since
    if (since > 0) {
        for (const auto& measurement : recentMeasurements) {
            if (measurement.timestamp > since) {
                newMeasurements.push_back(measurement);
            }
        }
    } else {
        // Если since не указан, возвращаем все из буфера
        newMeasurements = recentMeasurements;
    }

    // Сортируем по времени
    std::sort(newMeasurements.begin(), newMeasurements.end(),
        [](const LiveUpdate& a, const LiveUpdate& b) {
            return a.timestamp < b.timestamp;
        });

    std::ostringstream json;
    json << "{\"data\":[";

    for (size_t i = 0; i < newMeasurements.size(); i++) {
        if (i > 0) json << ",";
        json << "{\"timestamp\":\"" << timeToIsoString(newMeasurements[i].timestamp) << "\",";
        json << "\"temperature\":" << newMeasurements[i].temperature << "}";
    }

    json << "],";

    // Добавляем время последнего обновления
    if (!newMeasurements.empty()) {
        json << "\"lastUpdate\":\"" << timeToIsoString(newMeasurements.back().timestamp) << "\"";
    } else {
        json << "\"lastUpdate\":null";
    }

    json << "}";

    return json.str();
}

void WebServer::handleClient(int clientSocket) {
    char buffer[4096];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        close(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string request(buffer);

    // Парсим запрос
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;

    // Убираем query string из path
    size_t queryPos = path.find('?');
    std::string query = "";
    if (queryPos != std::string::npos) {
        query = path.substr(queryPos + 1);
        path = path.substr(0, queryPos);
    }

    std::string response;

    if (method == "GET") {
        if (path == "/api/current") {
            std::string json = handleApiCurrent();
            response = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: " + std::to_string(json.size()) + "\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n" + json;
        } else if (path == "/api/range") {
            std::string json = handleApiRange(query);
            response = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: " + std::to_string(json.size()) + "\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n" + json;
        } else if (path == "/api/stats") {
            std::string json = handleApiStats(query);
            response = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: " + std::to_string(json.size()) + "\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n" + json;
        } else if (path == "/api/live") {
            std::string json = handleApiLive(query);
            response = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: " + std::to_string(json.size()) + "\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n" + json;
        } else {
            // Статические файлы из web/ директории
            if (path == "/" || path.empty()) path = "/index.html";

            // Убираем начальный слеш
            if (path[0] == '/') path = path.substr(1);

            // Безопасно проверяем путь
            if (path.find("..") != std::string::npos) {
                response = "HTTP/1.1 403 Forbidden\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 13\r\n"
                          "\r\n"
                          "Forbidden";
            } else {
                // Ищем файл в web директории
                std::string filePath = "web/" + path;

                std::string content = readFile(filePath);

                if (content.empty()) {
                    // Если файл не найден, пробуем index.html
                    if (path != "index.html") {
                        filePath = "web/index.html";
                        content = readFile(filePath);
                    }

                    if (content.empty()) {
                        response = "HTTP/1.1 404 Not Found\r\n"
                                  "Content-Type: text/plain\r\n"
                                  "Content-Length: 13\r\n"
                                  "\r\n"
                                  "404 Not Found";
                    } else {
                        response = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: text/html; charset=utf-8\r\n"
                                  "Content-Length: " + std::to_string(content.size()) + "\r\n"
                                  "\r\n" + content;
                    }
                } else {
                    std::string contentType = getContentType(filePath);
                    response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: " + contentType + "\r\n"
                              "Content-Length: " + std::to_string(content.size()) + "\r\n"
                              "\r\n" + content;
                }
            }
        }
    } else {
        response = "HTTP/1.1 405 Method Not Allowed\r\n"
                  "Content-Type: text/plain\r\n"
                  "Content-Length: 18\r\n"
                  "\r\n"
                  "Method Not Allowed";
    }

    send(clientSocket, response.c_str(), response.size(), 0);
    close(clientSocket);
}

void WebServer::start() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
               (const char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка привязки сокета к порту " << port << std::endl;
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Ошибка прослушивания порта" << std::endl;
        close(serverSocket);
        return;
    }

    running = true;
    std::cout << "HTTP сервер запущен на порту " << port << std::endl;

    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);

        if (clientSocket < 0) {
            if (running) {
                std::cerr << "Ошибка принятия соединения" << std::endl;
            }
            continue;
        }

        // Обрабатываем запрос
        handleClient(clientSocket);
    }
}

void WebServer::stop() {
    running = false;
    if (serverSocket >= 0) {
#ifdef _WIN32
        shutdown(serverSocket, SD_BOTH);
#else
        shutdown(serverSocket, SHUT_RDWR);
#endif
        close(serverSocket);
        serverSocket = -1;
    }
}