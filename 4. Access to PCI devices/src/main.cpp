#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <deque>
#include <iomanip>
#include <thread>
#include <atomic>
#include <filesystem>
#include <csignal>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #include <cstdio>
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

void setup_streams() {
#ifdef _WIN32
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    SetConsoleOutputCP(CP_UTF8);
#endif
    // В Linux буферизация по умолчанию нормальная
}

void trim_file(const string& filename, time_t cutoff_time) {
    ifstream in_file(filename);
    if (!in_file) return;

    vector<string> recent_lines;
    string line;

    while (getline(in_file, line)) {
        if (line.empty()) continue;

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

                time_t file_time;
#ifdef _WIN32
                file_time = _mkgmtime(&tm_struct);
#else
                file_time = timegm(&tm_struct);
#endif

                if (file_time >= cutoff_time) {
                    recent_lines.push_back(line);
                }
            }
        }
    }
    in_file.close();

    ofstream out_file(filename);
    for (const auto& l : recent_lines) {
        out_file << l << "\n";
    }
}

int main() {
    setup_streams();
    setup_signal_handler();

    filesystem::create_directories("logs");

    string meas_file = "logs/measurements.log";
    string hour_file = "logs/hourly.log";
    string day_file = "logs/daily.log";

    time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    trim_file(meas_file, now - 24 * 3600);      // 24 часа
    trim_file(hour_file, now - 30 * 24 * 3600); // 30 дней

    tm current_tm;
#ifdef _WIN32
    gmtime_s(&current_tm, &now);
#else
    gmtime_r(&now, &current_tm);
#endif
    tm year_start_tm = {};
    year_start_tm.tm_year = current_tm.tm_year;
    year_start_tm.tm_mon = 0;
    year_start_tm.tm_mday = 1;
    year_start_tm.tm_hour = 0;
    year_start_tm.tm_min = 0;
    year_start_tm.tm_sec = 0;
    year_start_tm.tm_isdst = 0;

    time_t year_start;
#ifdef _WIN32
    year_start = _mkgmtime(&year_start_tm);
#else
    year_start = timegm(&year_start_tm);
#endif
    trim_file(day_file, year_start);

    ofstream meas(meas_file, ios::app);
    ofstream hourly(hour_file, ios::app);
    ofstream daily(day_file, ios::app);

    if (!meas || !hourly || !daily) {
        cerr << "Ошибка открытия файлов логов!" << endl;
        return 1;
    }

    cerr << "Температурный логгер запущен" << endl;
    cerr << "Файлы логов: " << meas_file << ", " << hour_file << ", " << day_file << endl;

    double hour_sum = 0, day_sum = 0;
    int hour_count = 0, day_count = 0;

    time_t current_hour = 0;
    time_t current_day = 0;

    vector<string> measurement_buffer;
    const int BUFFER_SIZE = 10; // Запись каждые 10 измерений

    string line;
    int total_count = 0;

    while (!stop_flag) {
        if (!getline(cin, line)) {
            this_thread::sleep_for(chrono::milliseconds(100));
            continue;
        }

        if (line.empty()) continue;

        size_t comma = line.find(',');
        if (comma == string::npos) continue;

        string time_str = line.substr(0, comma);
        string temp_str = line.substr(comma + 1);

        try {
            double temp = stod(temp_str);
            time_t timestamp = now;

            measurement_buffer.push_back(time_str + "," + to_string(temp));

            if (measurement_buffer.size() >= BUFFER_SIZE) {
                for (const auto& entry : measurement_buffer) {
                    meas << entry << "\n";
                }
                meas.flush();
                measurement_buffer.clear();
                cerr << "Записано " << BUFFER_SIZE << " измерений в measurements.log" << endl;
            }

            total_count++;

            if (current_hour == 0) current_hour = (timestamp / 3600) * 3600;
            if (current_day == 0) current_day = (timestamp / 86400) * 86400;

            hour_sum += temp;
            hour_count++;
            day_sum += temp;
            day_count++;

            time_t sample_hour = (timestamp / 3600) * 3600;
            if (sample_hour != current_hour && hour_count > 0) {
                double hour_avg = hour_sum / hour_count;
                hourly << time_to_string(current_hour) << "," << fixed << setprecision(3) << hour_avg << "\n";
                hourly.flush();

                trim_file(meas_file, now - 24 * 3600);
                trim_file(hour_file, now - 30 * 24 * 3600);

                hour_sum = hour_count = 0;
                current_hour = sample_hour;
                cerr << "Записано среднее за час: " << hour_avg << endl;
            }

            time_t sample_day = (timestamp / 86400) * 86400;
            if (sample_day != current_day && day_count > 0) {
                double day_avg = day_sum / day_count;
                daily << time_to_string(current_day) << "," << fixed << setprecision(3) << day_avg << "\n";
                daily.flush();

                trim_file(day_file, year_start);

                day_sum = day_count = 0;
                current_day = sample_day;
                cerr << "Записано среднее за день: " << day_avg << endl;
            }

        } catch (...) {
            cerr << "Ошибка обработки данных: " << line << endl;
        }
    }

    if (!measurement_buffer.empty()) {
        for (const auto& entry : measurement_buffer) {
            meas << entry << "\n";
        }
    }

    if (hour_count > 0) {
        double hour_avg = hour_sum / hour_count;
        hourly << time_to_string(current_hour) << "," << fixed << setprecision(3) << hour_avg << "\n";
    }

    if (day_count > 0) {
        double day_avg = day_sum / day_count;
        daily << time_to_string(current_day) << "," << fixed << setprecision(3) << day_avg << "\n";
    }

    cerr << "Программа завершена. Всего измерений: " << total_count << endl;
    return 0;
}