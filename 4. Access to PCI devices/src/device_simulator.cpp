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
    normal_distribution<> dist(23.5, 2.0); // Средняя 23.5°C, отклонение 2.0°C
    
    cout << "Симулятор температурного датчика" << endl;
    cout << "Формат: ISO8601,temperature" << endl;
    cout << "Пример: 2024-01-15T14:30:00Z,23.456" << endl;
    cout << "Для остановки нажмите Ctrl+C\n" << endl;
    
    while (true) {
        // Генерация температуры
        double temp = dist(gen);
        
        // Получение текущего времени в формате ISO8601
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        tm* timeinfo = gmtime(&now_time);
        
        char time_buffer[80];
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
        
        // Вывод в формате: ISO8601,temperature
        cout << time_buffer << "," << fixed << setprecision(3) << temp << endl;
        
        // Пауза между измерениями (200-800 мс)
        this_thread::sleep_for(chrono::milliseconds(200 + (rand() % 600)));
    }
    
    return 0;
}