#include "database.h"
#include "webserver.h"
#include "serial_port.h"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
#include <atomic>
#include <filesystem>
#include <csignal>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
#endif

using namespace std;
using namespace chrono;

atomic<bool> stop_flag{false};

void setup_signal_handler() {
#ifdef _WIN32
    signal(SIGINT, [](int){ stop_flag = true; });
#else
    struct sigaction sa;
    sa.sa_handler = [](int){ stop_flag = true; };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
#endif
}

string time_to_string(time_t t) {
    tm timeinfo;
#ifdef _WIN32
    gmtime_s(&timeinfo, &t);
#else
    gmtime_r(&t, &timeinfo);
#endif
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    return string(buffer);
}

time_t parse_iso_time(const string& isoTime) {
    tm tm_struct = {};
    if (sscanf(isoTime.c_str(), "%d-%d-%dT%d:%d:%dZ",
               &tm_struct.tm_year, &tm_struct.tm_mon, &tm_struct.tm_mday,
               &tm_struct.tm_hour, &tm_struct.tm_min, &tm_struct.tm_sec) != 6) {
        return 0;
    }
    tm_struct.tm_year -= 1900;
    tm_struct.tm_mon -= 1;
    tm_struct.tm_isdst = 0;

#ifdef _WIN32
    return _mkgmtime(&tm_struct);
#else
    return timegm(&tm_struct);
#endif
}

bool read_from_stdin(double& temp, time_t& timestamp, string& time_str) {
    string line;
    if (!getline(cin, line)) return false;

    size_t comma = line.find(',');
    if (comma == string::npos) return false;

    time_str = line.substr(0, comma);
    string temp_str = line.substr(comma + 1);

    timestamp = parse_iso_time(time_str);
    if (timestamp == 0) {
        timestamp = system_clock::to_time_t(system_clock::now());
        time_str = time_to_string(timestamp);
    }

    try {
        temp = stod(temp_str);
        return true;
    } catch (...) {
        return false;
    }
}

bool read_from_port(SerialPort& port, double& temp, time_t& timestamp, string& time_str) {
    string line;
    if (!port.read_line(line, 2000)) return false;

    size_t comma = line.find(',');
    if (comma != string::npos) {
        time_str = line.substr(0, comma);
        string temp_str = line.substr(comma + 1);

        timestamp = parse_iso_time(time_str);
        if (timestamp == 0) {
            timestamp = system_clock::to_time_t(system_clock::now());
            time_str = time_to_string(timestamp);
        }

        try {
            temp = stod(temp_str);
            return true;
        } catch (...) {
            return false;
        }
    } else {
        // Если нет запятой, считаем что это только температура
        timestamp = system_clock::to_time_t(system_clock::now());
        time_str = time_to_string(timestamp);

        try {
            temp = stod(line);
            return true;
        } catch (...) {
            return false;
        }
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    setup_signal_handler();

    string port_name;
    bool use_port = false;
    int http_port = 8080;
    string db_path = "temperature.db";

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port_name = argv[++i];
            use_port = true;
        } else if (arg == "--http-port" && i + 1 < argc) {
            http_port = stoi(argv[++i]);
        } else if (arg == "--db" && i + 1 < argc) {
            db_path = argv[++i];
        } else if (arg == "--help") {
            cout << "Использование:" << endl;
            cout << "  " << argv[0] << " [опции]" << endl;
            cout << "Опции:" << endl;
            cout << "  --port COMx         : Использовать последовательный порт" << endl;
            cout << "  --http-port N       : Порт HTTP сервера (по умолчанию: 8080)" << endl;
            cout << "  --db путь           : Путь к файлу базы данных" << endl;
            cout << "  --help              : Показать эту справку" << endl;
            return 0;
        }
    }

    // Инициализация базы данных
    Database db;
    if (!db.open(db_path)) {
        cerr << "Ошибка открытия базы данных!" << endl;
        return 1;
    }

    cout << "База данных открыта: " << db_path << endl;

    // Запуск HTTP сервера
    WebServer server(db, http_port);
    thread server_thread([&server]() {
        server.start();
    });

    // Даем серверу время на запуск
    this_thread::sleep_for(milliseconds(500));

    cout << "HTTP сервер запущен на порту " << http_port << endl;
    cout << "Веб-интерфейс доступен по адресу: http://localhost:" << http_port << endl;
    cout << "Для просмотра в реальном времени используйте /live.html" << endl;

    // Проверяем существование web директории
    if (!filesystem::exists("web")) {
        cout << "Предупреждение: директория web не найдена!" << endl;
        cout << "Создайте директорию web/ в папке с исполняемым файлом" << endl;
        cout << "И поместите туда файлы index.html, live.html, style.css, chart.js" << endl;
    }

    SerialPort serial_port;
    if (use_port) {
        if (!serial_port.open(port_name, 9600)) {
            cerr << "Ошибка: не удалось открыть порт " << port_name << endl;
            server.stop();
            if (server_thread.joinable()) server_thread.join();
            return 1;
        }
        cout << "Используется порт: " << port_name << endl;
    } else {
        cout << "Режим: чтение из stdin (симулятор)" << endl;
    }

    cout << "Температурный логгер запущен. Для остановки нажмите Ctrl+C" << endl;

    string time_str;
    double temp;
    time_t timestamp;
    int total_count = 0;
    time_t last_cleanup = system_clock::to_time_t(system_clock::now());

    while (!stop_flag) {
        bool success;

        if (use_port) {
            success = read_from_port(serial_port, temp, timestamp, time_str);
        } else {
            success = read_from_stdin(temp, timestamp, time_str);
        }

        if (!success) {
            this_thread::sleep_for(milliseconds(100));
            continue;
        }

        // Сохраняем в базу данных
        if (db.insertMeasurement(timestamp, temp)) {
            total_count++;

            // Добавляем измерение в сервер для live-обновлений
            server.addMeasurement(timestamp, temp);

            if (total_count % 10 == 0) {
                cout << "Записано измерений: " << total_count << endl;
            }
        } else {
            cerr << "Ошибка записи в базу данных!" << endl;
        }

        // Раз в час очищаем старые данные (старше 30 дней)
        time_t now = system_clock::to_time_t(system_clock::now());
        if (now - last_cleanup > 3600) { // 1 час
            db.cleanupOldData(30);
            last_cleanup = now;
        }

        // Небольшая пауза
        this_thread::sleep_for(milliseconds(50));
    }

    cout << "Остановка программы..." << endl;

    server.stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }

    db.close();

    cout << "Программа завершена. Всего измерений: " << total_count << endl;
    return 0;
}