#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>
#include <deque>
#include <iomanip>
#include <thread>
#include <atomic>
#include <filesystem>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
    #define timegm _mkgmtime
#else
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
#endif

using namespace std;
using namespace chrono;

atomic<bool> stop_flag{false};
filesystem::path log_dir = "logs";

// Получение времени начала часа и дня
time_t floor_hour(time_t t) { return (t / 3600) * 3600; }
time_t floor_day(time_t t) { return (t / 86400) * 86400; }

// Преобразование времени в строку
string time_to_string(time_t t) {
    tm* timeinfo = gmtime(&t);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
    return string(buffer);
}

// Чтение данных из stdin (от симулятора)
bool read_from_simulator(double& temp, time_t& timestamp) {
    string line;
    if (!getline(cin, line)) return false;

    size_t comma = line.find(',');
    if (comma == string::npos) return false;

    string time_str = line.substr(0, comma);
    string temp_str = line.substr(comma + 1);

    // Парсинг времени
    tm tm_struct = {};
    if (sscanf(time_str.c_str(), "%d-%d-%dT%d:%d:%dZ",
               &tm_struct.tm_year, &tm_struct.tm_mon, &tm_struct.tm_mday,
               &tm_struct.tm_hour, &tm_struct.tm_min, &tm_struct.tm_sec) != 6) return false;

    tm_struct.tm_year -= 1900;
    tm_struct.tm_mon -= 1;
    tm_struct.tm_isdst = 0;

    timestamp = timegm(&tm_struct);
    temp = stod(temp_str);
    return true;
}

// Очистка старых данных из файла
void trim_file(const string& filename, time_t cutoff) {
    ifstream in(filename);
    if (!in) return;

    deque<string> lines;
    string line;

    while (getline(in, line)) {
        size_t comma = line.find(',');
        if (comma != string::npos) {
            string time_str = line.substr(0, comma);
            tm tm_struct = {};
            if (sscanf(time_str.c_str(), "%d-%d-%dT%d:%d:%dZ",
                      &tm_struct.tm_year, &tm_struct.tm_mon, &tm_struct.tm_mday,
                      &tm_struct.tm_hour, &tm_struct.tm_min, &tm_struct.tm_sec) == 6) {
                tm_struct.tm_year -= 1900;
                tm_struct.tm_mon -= 1;
                tm_struct.tm_isdst = 0;
                time_t file_time = timegm(&tm_struct);
                if (file_time >= cutoff) {
                    lines.push_back(line);
                }
            }
        }
    }
    in.close();

    ofstream out(filename);
    for (const auto& l : lines) out << l << "\n";
}

// Получение времени начала года
time_t get_year_start(time_t t) {
    tm* timeinfo = gmtime(&t);
    tm year_start = {};
    year_start.tm_year = timeinfo->tm_year;
    year_start.tm_mday = 1;
    year_start.tm_mon = 0;
    year_start.tm_hour = 0;
    year_start.tm_min = 0;
    year_start.tm_sec = 0;
    year_start.tm_isdst = 0;
    return timegm(&year_start);
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    signal(SIGINT, [](int){ stop_flag = true; });

    bool simulate = true;
    string port_name;

    // Парсинг аргументов
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--port" && i+1 < argc) {
            port_name = argv[++i];
            simulate = false;
        } else if (arg == "--simulate") {
            simulate = true;
        } else if (arg == "--log-dir" && i+1 < argc) {
            log_dir = argv[++i];
        } else if (arg == "--help") {
            cout << "Usage: " << argv[0] << " [--port COMx] [--simulate] [--log-dir DIR]\n";
            return 0;
        }
    }

    // Создание директории для логов
    filesystem::create_directories(log_dir);

    string meas_file = (log_dir / "measurements.log").string();
    string hour_file = (log_dir / "hourly.log").string();
    string day_file = (log_dir / "daily.log").string();

    // Инициализация файлов
    ofstream(meas_file, ios::app).close();
    ofstream(hour_file, ios::app).close();
    ofstream(day_file, ios::app).close();

    // Начальное время
    time_t start_time = chrono::system_clock::to_time_t(system_clock::now());
    time_t current_hour = floor_hour(start_time);
    time_t current_day = floor_day(start_time);

    // Аккумуляторы для средних
    double hour_sum = 0, day_sum = 0;
    int hour_count = 0, day_count = 0;

    // Буфер для измерений
    vector<string> buffer;

    cout << "Температурный логгер запущен\n";
    if (simulate) cout << "Режим: симулятор\n";
    else cout << "Режим: порт " << port_name << "\n";

    while (!stop_flag) {
        double temp;
        time_t timestamp;

        if (!read_from_simulator(temp, timestamp)) {
            this_thread::sleep_for(chrono::milliseconds(100));
            continue;
        }

        // Запись в буфер
        buffer.push_back(time_to_string(timestamp) + "," + to_string(temp));

        // Запись буфера в файл (каждые 10 измерений)
        if (buffer.size() >= 10) {
            ofstream out(meas_file, ios::app);
            for (const auto& line : buffer) out << line << "\n";
            buffer.clear();
            cout << "Записано 10 измерений в measurements.log\n";
        }

        // Обновление аккумуляторов
        hour_sum += temp; hour_count++;
        day_sum += temp; day_count++;

        time_t sample_hour = floor_hour(timestamp);
        time_t sample_day = floor_day(timestamp);

        // Проверка смены часа
        if (sample_hour != current_hour && hour_count > 0) {
            double hour_avg = hour_sum / hour_count;
            ofstream(hour_file, ios::app)
                << time_to_string(current_hour) << "," << hour_avg << "\n";

            // Очистка старых данных
            trim_file(meas_file, chrono::system_clock::to_time_t(system_clock::now()) - 24*3600);
            trim_file(hour_file, chrono::system_clock::to_time_t(system_clock::now()) - 30*24*3600);

            hour_sum = hour_count = 0;
            current_hour = sample_hour;
            cout << "Записано среднее за час: " << hour_avg << "\n";
        }

        // Проверка смены дня
        if (sample_day != current_day && day_count > 0) {
            double day_avg = day_sum / day_count;
            ofstream(day_file, ios::app)
                << time_to_string(current_day) << "," << day_avg << "\n";

            // Очистка daily.log - оставляем только текущий год
            time_t year_start = get_year_start(timestamp);
            trim_file(day_file, year_start);

            day_sum = day_count = 0;
            current_day = sample_day;
            cout << "Записано среднее за день: " << day_avg << "\n";
        }
    }

    // Запись оставшихся данных в буфере
    if (!buffer.empty()) {
        ofstream out(meas_file, ios::app);
        for (const auto& line : buffer) out << line << "\n";
    }

    // Запись последних средних значений
    if (hour_count > 0) {
        ofstream(hour_file, ios::app)
            << time_to_string(current_hour) << "," << (hour_sum / hour_count) << "\n";
    }
    if (day_count > 0) {
        ofstream(day_file, ios::app)
            << time_to_string(current_day) << "," << (day_sum / day_count) << "\n";
    }

    cout << "Программа завершена\n";
    return 0;
}