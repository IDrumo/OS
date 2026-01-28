// simulator.cpp
#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <ctime>
#include <cmath>

#ifdef _WIN32
    #include <windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> noise_dist(-0.5, 0.5); // Случайный шум
    std::uniform_real_distribution<> spike_dist(0.0, 1.0); // Для выбросов

    double base_temp = 22.0; // Базовая температура
    double amplitude = 5.0;  // Амплитуда колебаний
    double period = 0.01 * 60 * 60;  // Период в секундах (1 час)

    int seconds = 0; // Счетчик секунд

    // Медленно меняющиеся параметры
    double slow_amplitude_change = 0.0;
    double slow_base_change = 0.0;
    double slow_period_change = 0.0;

    const double MAX_TEMP = 40.0;
    const double MIN_TEMP = 10.0;

    while (true) {
        // Медленно меняем параметры (раз в 30 секунд)
        if (seconds % 30 == 0) {
            std::normal_distribution<> param_change(0.0, 0.5);
            slow_amplitude_change += param_change(gen) * 0.1;
            slow_base_change += param_change(gen) * 0.05;
            slow_period_change += param_change(gen) * 10;

            // Ограничиваем изменения
            amplitude = std::max(2.0, std::min(8.0, 5.0 + slow_amplitude_change));
            base_temp = std::max(18.0, std::min(26.0, 22.0 + slow_base_change));
            period = std::max(1800.0, std::min(7200.0, 3600.0 + slow_period_change));
        }

        // Вычисляем температуру по синусоиде
        double time_factor = 2.0 * M_PI * seconds / period;
        double sine_value = std::sin(time_factor);
        double temp = base_temp + amplitude * sine_value + noise_dist(gen);

        // Иногда (0.5% вероятность) добавляем небольшой выброс
        if (spike_dist(gen) < 0.005) {
            std::normal_distribution<> spike_size(0.0, 3.0);
            temp += spike_size(gen);
            std::cerr << "Выброс: " << temp << "°C" << std::endl;
        }

        // Валидация температуры
        if (temp < MIN_TEMP || temp > MAX_TEMP) {
            // Плавно возвращаем к нормальному диапазону
            if (temp < MIN_TEMP) temp = MIN_TEMP + 0.5;
            else if (temp > MAX_TEMP) temp = MAX_TEMP - 0.5;
        }

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

        seconds++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}