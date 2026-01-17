#include "../include/serial.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>  // для std::rand
#include <cstring>  // для std::sprintf

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

SerialPort::SerialPort() = default;

SerialPort::~SerialPort() {
    if (is_open()) {
        close();
    }
}

SerialPort::SerialPort(SerialPort&& other) noexcept {
#ifdef _WIN32
    handle_ = other.handle_;
    other.handle_ = INVALID_HANDLE_VALUE;
#else
    fd_ = other.fd_;
    other.fd_ = -1;
#endif
    is_open_ = other.is_open_;
    other.is_open_ = false;
}

SerialPort& SerialPort::operator=(SerialPort&& other) noexcept {
    if (this != &other) {
        close();
#ifdef _WIN32
        handle_ = other.handle_;
        other.handle_ = INVALID_HANDLE_VALUE;
#else
        fd_ = other.fd_;
        other.fd_ = -1;
#endif
        is_open_ = other.is_open_;
        other.is_open_ = false;
    }
    return *this;
}

bool SerialPort::open(const std::string& port, BaudRate baudrate) {
    close();

    std::cout << "Opening port: " << port << " at " << baudrate << " baud" << std::endl;

    // Заглушка для тестирования - всегда успешно
    is_open_ = true;

    std::cout << "Port opened successfully (simulation mode)" << std::endl;
    return true;
}

void SerialPort::close() {
    if (is_open_) {
        std::cout << "Closing port" << std::endl;
#ifdef _WIN32
        if (handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
#else
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
#endif
        is_open_ = false;
    }
}

bool SerialPort::is_open() const {
    return is_open_;
}

std::string SerialPort::read_line(char delimiter) {
    (void)delimiter;  // Подавляем предупреждение о неиспользуемом параметре

    if (!is_open()) {
        return "";
    }

    // Заглушка для тестирования - генерируем тестовые данные
    static int counter = 0;
    static double temperature = 22.0;

    counter++;

    // Имитируем реальные данные с небольшими колебаниями
    temperature += (std::rand() % 100 - 50) / 100.0; // ±0.5°C
    if (temperature < -40.0) temperature = -40.0;
    if (temperature > 60.0) temperature = 60.0;

    // Генерируем строку с контрольной суммой
    std::string temp_str = std::to_string(temperature);
    unsigned char checksum = 0;
    for (char c : temp_str) {
        checksum ^= static_cast<unsigned char>(c);
    }

    char checksum_str[3];
    std::sprintf(checksum_str, "%02X", checksum);

    std::string result = std::to_string(temperature).substr(0, 5) + " " + checksum_str;

    // Имитируем задержку чтения
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return result;
}

void SerialPort::write_string(const std::string& str) {
    if (!is_open()) {
        return;
    }

    std::cout << "Writing to port: " << str;
}

void SerialPort::set_timeout(double seconds) {
    std::cout << "Setting timeout to " << seconds << " seconds" << std::endl;
}

void SerialPort::flush() {
    std::cout << "Flushing port buffers" << std::endl;
}