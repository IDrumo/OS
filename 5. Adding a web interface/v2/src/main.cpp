#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
#include <atomic>
#include <vector>
#include <sstream>
#include <sqlite3.h>

#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <signal.h>
#endif

using namespace std;
using namespace chrono;

atomic<bool> stop_flag{false};

// ========== БАЗА ДАННЫХ ==========
class Database {
    sqlite3* db = nullptr;
public:
    bool open(const string& path) {
        int rc = sqlite3_open(path.c_str(), &db);
        if (rc != SQLITE_OK) return false;

        const char* sql = "CREATE TABLE IF NOT EXISTS measurements ("
                         "timestamp INTEGER PRIMARY KEY, "
                         "temperature REAL NOT NULL);";
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
        return true;
    }

    void close() { if(db) sqlite3_close(db); }

    bool insert(time_t timestamp, double temperature) {
        const char* sql = "INSERT INTO measurements VALUES(?, ?);";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, timestamp);
        sqlite3_bind_double(stmt, 2, temperature);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }

    string getLatestJson() {
        const char* sql = "SELECT timestamp, temperature FROM measurements "
                         "ORDER BY timestamp DESC LIMIT 1;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

        ostringstream json;
        json << "{";

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            time_t ts = sqlite3_column_int64(stmt, 0);
            double temp = sqlite3_column_double(stmt, 1);

            // Форматируем время
            tm timeinfo;
            #ifdef _WIN32
                gmtime_s(&timeinfo, &ts);
            #else
                gmtime_r(&ts, &timeinfo);
            #endif
            char buffer[80];
            strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

            json << "\"timestamp\":\"" << buffer << "\",";
            json << "\"temperature\":" << fixed << setprecision(2) << temp;
        } else {
            json << "\"timestamp\":null,\"temperature\":null";
        }

        json << "}";
        sqlite3_finalize(stmt);
        return json.str();
    }

    string getRangeJson(time_t from, time_t to) {
        const char* sql = "SELECT timestamp, temperature FROM measurements "
                         "WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, from);
        sqlite3_bind_int64(stmt, 2, to);

        ostringstream json;
        json << "{\"data\":[";

        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) json << ",";
            first = false;

            time_t ts = sqlite3_column_int64(stmt, 0);
            double temp = sqlite3_column_double(stmt, 1);

            tm timeinfo;
            #ifdef _WIN32
                gmtime_s(&timeinfo, &ts);
            #else
                gmtime_r(&ts, &timeinfo);
            #endif
            char buffer[80];
            strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

            json << "{\"t\":\"" << buffer << "\",\"v\":" << fixed << setprecision(2) << temp << "}";
        }

        json << "]}";
        sqlite3_finalize(stmt);
        return json.str();
    }

    // Метод для проверки наличия данных
    bool hasData() {
        const char* sql = "SELECT COUNT(*) FROM measurements;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        bool hasData = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            hasData = sqlite3_column_int(stmt, 0) > 0;
        }
        sqlite3_finalize(stmt);
        return hasData;
    }

    ~Database() { close(); }
};

// ========== HTTP СЕРВЕР ==========
class WebServer {
    Database& db;
    int port;
    int serverSocket;
    atomic<bool> running{false};

    string readFile(const string& path) {
        ifstream file(path, ios::binary);
        if (!file) return "";
        ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    string getContentType(const string& path) {
        if (path.find(".html") != string::npos) return "text/html; charset=utf-8";
        if (path.find(".js") != string::npos) return "application/javascript";
        if (path.find(".css") != string::npos) return "text/css";
        return "text/plain";
    }

    time_t parseTime(const string& str) {
        string decoded;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '%' && i + 2 < str.size()) {
                int value = 0;
                for (int j = 1; j <= 2; ++j) {
                    char c = tolower(str[i + j]);
                    value <<= 4;
                    if (c >= '0' && c <= '9') value += c - '0';
                    else if (c >= 'a' && c <= 'f') value += c - 'a' + 10;
                }
                decoded += static_cast<char>(value);
                i += 2;
            } else {
                decoded += str[i];
            }
        }

        // Debug output
        cout << "Parsing time: " << decoded << endl;

        // Пробуем разные форматы
        tm tm_struct = {};
        int year, month, day, hour, minute, second;

        // Формат: 2026-01-19T01:57:00Z (с секундами)
        int result = sscanf(decoded.c_str(), "%d-%d-%dT%d:%d:%dZ",
                           &year, &month, &day, &hour, &minute, &second);

        if (result == 6) {
            tm_struct.tm_year = year - 1900;
            tm_struct.tm_mon = month - 1;
            tm_struct.tm_mday = day;
            tm_struct.tm_hour = hour;
            tm_struct.tm_min = minute;
            tm_struct.tm_sec = second;
            tm_struct.tm_isdst = -1;

            cout << "Parsed as full format: " << year << "-" << month << "-" << day
                 << " " << hour << ":" << minute << ":" << second << endl;
        }
        // Формат: 2026-01-19T01:57 (без секунд)
        else if (sscanf(decoded.c_str(), "%d-%d-%dT%d:%d",
                        &year, &month, &day, &hour, &minute) == 5) {
            tm_struct.tm_year = year - 1900;
            tm_struct.tm_mon = month - 1;
            tm_struct.tm_mday = day;
            tm_struct.tm_hour = hour;
            tm_struct.tm_min = minute;
            tm_struct.tm_sec = 0;
            tm_struct.tm_isdst = -1;

            cout << "Parsed as no-seconds format: " << year << "-" << month << "-" << day
                 << " " << hour << ":" << minute << endl;
        }
        else {
            cout << "Failed to parse time string" << endl;
            return 0;
        }

        #ifdef _WIN32
            time_t timestamp = _mkgmtime(&tm_struct);
        #else
            time_t timestamp = timegm(&tm_struct);
        #endif

        if (timestamp == -1) {
            cout << "Time conversion failed" << endl;
            return 0;
        }

        cout << "Converted to timestamp: " << timestamp << endl;
        return timestamp;
    }

    void handleClient(int clientSocket) {
        char buffer[4096];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) { close(clientSocket); return; }
        buffer[bytesRead] = '\0';

        string request(buffer);
        cout << "\n=== HTTP Request ===" << endl;
        cout << request << endl;
        cout << "===================" << endl;

        istringstream iss(request);
        string method, path, http_version;
        iss >> method >> path >> http_version;

        string response;

        if (method == "GET") {
            if (path == "/api/current") {
                string json = db.getLatestJson();
                response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "Access-Control-Allow-Origin: *\r\n"
                          "Content-Length: " + to_string(json.size()) + "\r\n"
                          "\r\n" + json;
            }
            else if (path.find("/api/range") == 0) {
                size_t q = path.find('?');
                if (q != string::npos) {
                    string query = path.substr(q + 1);
                    size_t fromPos = query.find("from=");
                    size_t toPos = query.find("&to=");

                    if (fromPos != string::npos && toPos != string::npos) {
                        string fromStr = query.substr(fromPos + 5, toPos - fromPos - 5);
                        string toStr = query.substr(toPos + 4);

                        cout << "From: " << fromStr << endl;
                        cout << "To: " << toStr << endl;

                        time_t from = parseTime(fromStr);
                        time_t to = parseTime(toStr);

                        if (from && to) {
                            string json = db.getRangeJson(from, to);
                            response = "HTTP/1.1 200 OK\r\n"
                                      "Content-Type: application/json\r\n"
                                      "Access-Control-Allow-Origin: *\r\n"
                                      "Content-Length: " + to_string(json.size()) + "\r\n"
                                      "\r\n" + json;
                            cout << "Response JSON length: " << json.size() << endl;
                            cout << "Response JSON preview: " << json.substr(0, min((size_t)200, json.size())) << "..." << endl;
                        } else {
                            response = "HTTP/1.1 400 Bad Request\r\n"
                                      "Content-Type: text/plain\r\n"
                                      "Content-Length: 26\r\n"
                                      "\r\n"
                                      "Invalid time format provided";
                            cout << "Invalid time format" << endl;
                        }
                    } else {
                        response = "HTTP/1.1 400 Bad Request\r\n"
                                  "Content-Type: text/plain\r\n"
                                  "Content-Length: 17\r\n"
                                  "\r\n"
                                  "Missing parameters";
                        cout << "Missing parameters" << endl;
                    }
                } else {
                    response = "HTTP/1.1 400 Bad Request\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 17\r\n"
                              "\r\n"
                              "Missing query string";
                    cout << "Missing query string" << endl;
                }
            }
            else {
                // Статический файл
                if (path == "/") path = "/index.html";
                string filePath = "." + path;
                string content = readFile(filePath);

                if (content.empty()) {
                    response = "HTTP/1.1 404 Not Found\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 13\r\n"
                              "\r\n"
                              "404 Not Found";
                } else {
                    response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: " + getContentType(filePath) + "\r\n"
                              "Content-Length: " + to_string(content.size()) + "\r\n"
                              "\r\n" + content;
                }
            }
        }

        cout << "Sending response (" << response.size() << " bytes)" << endl;
        send(clientSocket, response.c_str(), response.size(), 0);
        close(clientSocket);
    }

public:
    WebServer(Database& db, int port = 8080) : db(db), port(port), serverSocket(-1) {
        #ifdef _WIN32
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2,2), &wsaData);
        #endif
    }

    ~WebServer() {
        stop();
        #ifdef _WIN32
            WSACleanup();
        #endif
    }

    void start() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) return;

        int opt = 1;
        #ifdef _WIN32
            setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
        #else
            setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        #endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
            close(serverSocket);
            return;
        }

        listen(serverSocket, 10);
        running = true;
        cout << "Сервер запущен: http://localhost:" << port << endl;

        while (running) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
            if (clientSocket >= 0) {
                thread([this, clientSocket]() { handleClient(clientSocket); }).detach();
            }
        }
    }

    void stop() {
        running = false;
        if (serverSocket >= 0) {
            close(serverSocket);
            serverSocket = -1;
        }
    }
};

// Функция для добавления тестовых данных
void addTestData(Database& db) {
    time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());

    // Добавляем данные за последние 24 часа
    for (int i = 0; i < 24; i++) {
        time_t ts = now - (23 - i) * 3600; // Последние 24 часа
        double temp = 20.0 + (rand() % 1000) / 100.0; // Случайная температура 20-30°C
        db.insert(ts, temp);
        cout << "Added test data: " << ts << " -> " << temp << "°C" << endl;
    }
}

// ========== ОСНОВНАЯ ПРОГРАММА ==========
int main(int argc, char* argv[]) {
    #ifdef _WIN32
        SetConsoleOutputCP(65001);
    #endif

    // Установка обработчика прерывания
    #ifdef _WIN32
        SetConsoleCtrlHandler([](DWORD event) -> BOOL {
            if (event == CTRL_C_EVENT) {
                stop_flag = true;
                return TRUE;
            }
            return FALSE;
        }, TRUE);
    #else
        signal(SIGINT, [](int){ stop_flag = true; });
    #endif

    int http_port = 8080;
    string db_path = "temp.db";

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--http-port" && i + 1 < argc) http_port = stoi(argv[++i]);
        else if (arg == "--db" && i + 1 < argc) db_path = argv[++i];
        else if (arg == "--help") {
            cout << "Использование: temp_logger [--http-port N] [--db файл.db]" << endl;
            return 0;
        }
    }

    // База данных
    Database db;
    if (!db.open(db_path)) {
        cerr << "Ошибка открытия БД!" << endl;
        return 1;
    }

    // Проверяем, есть ли данные
    if (!db.hasData()) {
        cout << "База данных пуста, добавляем тестовые данные..." << endl;
        addTestData(db);
    }

    // HTTP сервер
    WebServer server(db, http_port);
    thread server_thread([&server]() { server.start(); });
    this_thread::sleep_for(milliseconds(500));

    cout << "Чтение данных из stdin (формат: ISO8601,температура)" << endl;
    cout << "Пример: 2024-01-15T14:30:00Z,23.45" << endl;
    cout << "Нажмите Ctrl+C для остановки" << endl;

    string line;
    int count = 0;

    while (!stop_flag) {
        if (cin.peek() != EOF && getline(cin, line)) {
            size_t comma = line.find(',');
            if (comma == string::npos) continue;

            string time_str = line.substr(0, comma);
            string temp_str = line.substr(comma + 1);

            // Парсим время
            tm tm_struct = {};
            if (sscanf(time_str.c_str(), "%d-%d-%dT%d:%d:%dZ",
                       &tm_struct.tm_year, &tm_struct.tm_mon, &tm_struct.tm_mday,
                       &tm_struct.tm_hour, &tm_struct.tm_min, &tm_struct.tm_sec) != 6) {
                continue;
            }
            tm_struct.tm_year -= 1900;
            tm_struct.tm_mon -= 1;
            tm_struct.tm_isdst = -1;

            time_t timestamp;
            #ifdef _WIN32
                timestamp = _mkgmtime(&tm_struct);
            #else
                timestamp = timegm(&tm_struct);
            #endif

            try {
                double temp = stod(temp_str);
                if (db.insert(timestamp, temp)) {
                    count++;
                    if (count % 10 == 0) cout << "Измерений: " << count << endl;
                }
            } catch (...) {
                // Игнорируем ошибки преобразования
            }
        } else {
            this_thread::sleep_for(milliseconds(100));
        }
    }
    
    cout << "Остановка..." << endl;
    server.stop();
    if (server_thread.joinable()) server_thread.join();
    
    return 0;
}