#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    std::mt19937 gen(std::random_device{}());
    std::normal_distribution<> dist(23.5, 2.0);

    while (true) {
        // Генерируем температуру
        double temp = dist(gen);

        // Получаем текущее время
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);

        // Форматируем как ISO8601
        std::tm timeinfo;
#ifdef _WIN32
        gmtime_s(&timeinfo, &now_time);
#else
        gmtime_r(&now_time, &timeinfo);
#endif

        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

        // Выводим
        std::cout << buffer << "," << temp << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}