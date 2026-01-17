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

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #include <cstdio>
#endif

using namespace std;
using namespace chrono;

atomic<bool> stop_flag{false};

// Простое преобразование времени в строку
string time_to_string(time_t t) {
    tm* timeinfo = gmtime(&t);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
    return string(buffer);
}

int main() {
#ifdef _WIN32
    // Для PowerShell: отключаем буферизацию
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // Устанавливаем UTF-8
    SetConsoleOutputCP(CP_UTF8);
#endif

    signal(SIGINT, [](int){ stop_flag = true; });

    // Создаем директорию для логов
    filesystem::create_directories("logs");

    string meas_file = "logs/measurements.log";
    string hour_file = "logs/hourly.log";
    string day_file = "logs/daily.log";

    // Открываем файлы для записи
    ofstream meas(meas_file, ios::app);
    ofstream hourly(hour_file, ios::app);
    ofstream daily(day_file, ios::app);

    if (!meas || !hourly || !daily) {
        cerr << "Ошибка открытия файлов логов!" << endl;
        return 1;
    }

    cerr << "Логгер запущен. Ожидание данных..." << endl;

    // Аккумуляторы для средних значений
    double hour_sum = 0, day_sum = 0;
    int hour_count = 0, day_count = 0;

    // Временные метки начала текущего часа и дня
    time_t current_hour = 0;
    time_t current_day = 0;

    string line;
    int total_count = 0;

    // Читаем данные построчно
    while (!stop_flag) {
        if (!getline(cin, line)) {
            // Если поток закрыт, делаем небольшую паузу
            this_thread::sleep_for(chrono::milliseconds(100));
            continue;
        }

        if (line.empty()) continue;

        // Разбираем строку: ISO8601,temperature
        size_t comma = line.find(',');
        if (comma == string::npos) continue;

        string time_str = line.substr(0, comma);
        string temp_str = line.substr(comma + 1);

        try {
            double temp = stod(temp_str);
            time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());

            // Записываем измерение
            meas << time_str << "," << fixed << setprecision(3) << temp << "\n";
            meas.flush();

            // Обновляем счетчики
            total_count++;

            // Обновляем аккумуляторы
            if (current_hour == 0) current_hour = (now / 3600) * 3600;
            if (current_day == 0) current_day = (now / 86400) * 86400;

            hour_sum += temp;
            hour_count++;
            day_sum += temp;
            day_count++;

            // Проверяем смену часа
            time_t sample_hour = (now / 3600) * 3600;
            if (sample_hour != current_hour && hour_count > 0) {
                double hour_avg = hour_sum / hour_count;
                hourly << time_to_string(current_hour) << "," << fixed << setprecision(3) << hour_avg << "\n";
                hourly.flush();

                hour_sum = hour_count = 0;
                current_hour = sample_hour;
                cerr << "Записано среднее за час: " << hour_avg << endl;
            }

            // Проверяем смену дня
            time_t sample_day = (now / 86400) * 86400;
            if (sample_day != current_day && day_count > 0) {
                double day_avg = day_sum / day_count;
                daily << time_to_string(current_day) << "," << fixed << setprecision(3) << day_avg << "\n";
                daily.flush();

                day_sum = day_count = 0;
                current_day = sample_day;
                cerr << "Записано среднее за день: " << day_avg << endl;
            }

            // Выводим прогресс каждые 10 измерений
            if (total_count % 10 == 0) {
                cerr << "Принято измерений: " << total_count << endl;
            }

        } catch (...) {
            cerr << "Ошибка парсинга строки: " << line << endl;
        }
    }

    // Записываем оставшиеся средние значения
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