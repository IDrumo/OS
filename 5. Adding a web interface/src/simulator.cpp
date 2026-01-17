#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
#endif

using namespace std;

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    random_device rd;
    mt19937 gen(rd());
    normal_distribution<> dist(23.5, 2.0);

    cerr << "Симулятор температурного датчика" << endl;
    cerr << "Формат: ISO8601,temperature" << endl;
    cerr << "Пример: 2024-01-15T14:30:00Z,23.456" << endl;
    cerr << "Для остановки нажмите Ctrl+C" << endl;

    while (true) {
        double temp = dist(gen);

        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);

        tm timeinfo;
#ifdef _WIN32
        gmtime_s(&timeinfo, &now_time);
#else
        gmtime_r(&now_time, &timeinfo);
#endif

        char time_buffer[80];
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

        // Выводим в stdout для пайпа
        cout << time_buffer << "," << fixed << setprecision(3) << temp << endl;
        cout.flush();

        this_thread::sleep_for(chrono::milliseconds(500));
    }
    
    return 0;
}